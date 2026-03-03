#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/atomic.h>

/* 硬编码寄存器基址及大小 */
#define STIMER_BASE_ADDR   0x10011000UL
#define STIMER_MEM_SIZE    0x1000

/* DW APB Timer 寄存器偏移 */
#define STIMER_LOAD        0x00   /* 重载值寄存器 */
#define STIMER_CTRL        0x08   /* 控制寄存器 */
#define STIMER_INTCLR      0x0C   /* 中断清除寄存器 */

/* ioctl 命令 */
#define STIMER_SET_INTERVAL _IOW('s', 1, unsigned int)
#define STIMER_START        _IO('s', 2)   /* 启动定时器（配置并开始） */
#define STIMER_STOP         _IO('s', 3)   /* 停止定时器 */

static void __iomem *timer_regs;
static atomic_t timer_triggered = ATOMIC_INIT(0);
static wait_queue_head_t timer_wait_queue;
static unsigned int timer_interval = 80000;   /* 默认周期 */

/* 中断处理函数（上半部） */
static irqreturn_t stimer_irq_handler(int irq, void *dev_id)
{
    /* 清除中断（读 INTCLR 寄存器） */
    readl(timer_regs + STIMER_INTCLR);

    atomic_set(&timer_triggered, 1);
    wake_up_interruptible(&timer_wait_queue);
    /* 注意：此处不能将 timer_triggered 清零，否则唤醒的进程可能看不到标志 */
    return IRQ_HANDLED;
}

/* read：仅阻塞等待下一次中断 */
static ssize_t stimer_read(struct file *file, char __user *buf,
                           size_t count, loff_t *ppos)
{
    /* 等待中断事件 */
    if (wait_event_interruptible(timer_wait_queue, atomic_read(&timer_triggered)))
        return -ERESTARTSYS;

    /* 读取后自动清除标志，以便下次等待 */
    atomic_set(&timer_triggered, 0);
    return 0;
}

/* ioctl：支持设置周期、启动、停止 */
static long stimer_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned int val;

    switch (cmd) {
    case STIMER_SET_INTERVAL:
        if (copy_from_user(&val, (unsigned int __user *)arg, sizeof(val)))
            return -EFAULT;
        if (val == 0)
            return -EINVAL;
        timer_interval = val;
        pr_info("STimer: interval updated to %u\n", timer_interval);
        break;

    case STIMER_START:
        /* 1. 禁用定时器，设置用户定义模式（写 0x2） */
        writel(0x2, timer_regs + STIMER_CTRL);

        /* 2. 清除硬件中断（读 INTCLR） */
        readl(timer_regs + STIMER_INTCLR);

        /* 3. 设置周期 */
        writel(timer_interval, timer_regs + STIMER_LOAD);

        /* 4. 清除软件事件标志 */
        atomic_set(&timer_triggered, 0);

        /* 5. 使能定时器（用户定义模式 + 中断使能，写 0x3） */
        writel(0x3, timer_regs + STIMER_CTRL);

        pr_info("STimer: started with interval %u\n", timer_interval);
        break;

    case STIMER_STOP:
        /* 停止定时器：写 0x2（禁用，保持模式） */
        writel(0x2, timer_regs + STIMER_CTRL);
        pr_info("STimer: stopped\n");
        break;

    default:
        return -EINVAL;
    }
    return 0;
}

/* file_operations */
static const struct file_operations stimer_fops = {
    .owner          = THIS_MODULE,
    .read           = stimer_read,
    .unlocked_ioctl = stimer_ioctl,
};

static struct miscdevice stimer_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "stimer",
    .fops  = &stimer_fops,
};

/* 设备树匹配表 */
static const struct of_device_id stimer_of_match[] = {
    { .compatible = "hybrid0,sideband" },  
    { /* end */ }
};
MODULE_DEVICE_TABLE(of, stimer_of_match);

/* probe 函数 */
static int stimer_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    int irq, ret, i = 0;

    /* 1. 映射寄存器（硬编码地址） */
    timer_regs = devm_ioremap(dev, STIMER_BASE_ADDR, STIMER_MEM_SIZE);
    if (!timer_regs) {
        dev_err(dev, "ioremap failed\n");
        return -ENOMEM;
    }

    /* 2. 获取并注册所有中断（统一处理） */
    while ((irq = irq_of_parse_and_map(dev->of_node, i++)) != 0) {
        ret = devm_request_irq(dev, irq, stimer_irq_handler,
                               IRQF_TRIGGER_RISING,   /* 根据硬件调整触发方式 */
                               "stimer", NULL);
        if (ret) {
            dev_warn(dev, "request_irq %d failed: %d, skip\n", irq, ret);
            irq_dispose_mapping(irq);
            continue;
        }
        dev_info(dev, "registered IRQ %d\n", irq);
    }

    /* 3. 初始化等待队列 */
    init_waitqueue_head(&timer_wait_queue);

    /* 4. 注册 misc 设备 */
    ret = misc_register(&stimer_miscdev);
    if (ret) {
        dev_err(dev, "misc_register failed\n");
        return ret;
    }

    platform_set_drvdata(pdev, &stimer_miscdev);
    dev_info(dev, "STimer loaded, base=0x%lx\n", STIMER_BASE_ADDR);
    return 0;
}

/* remove 函数 */
static int stimer_remove(struct platform_device *pdev)
{
    misc_deregister(&stimer_miscdev);
    return 0;
}

/* platform_driver 结构 */
static struct platform_driver stimer_driver = {
    .probe  = stimer_probe,
    .remove = stimer_remove,
    .driver = {
        .name           = "stimer",
        .of_match_table = stimer_of_match,
    },
};

module_platform_driver(stimer_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple STimer driver with start/stop ioctl");