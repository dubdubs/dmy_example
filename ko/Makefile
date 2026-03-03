# 指定要编译的模块
obj-m += hybrid_stimer.o

# 交叉编译工具链路径（根据实际情况调整）
PATH := /Xuantie-900-gcc-linux-6.6.0-glibc-x86_64-V3.2.0/bin:$(PATH)
# 内核源码路径
KDIR := /kernel/linux-6.6.36
# 交叉编译前缀
CROSS_COMPILE = riscv64-unknown-linux-gnu-

# 当前目录
PWD := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) ARCH=riscv CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
