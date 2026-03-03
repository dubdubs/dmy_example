#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/select.h>

#define STIMER_SET_INTERVAL _IOW('s', 1, unsigned int)
#define STIMER_START        _IO('s', 2)   // 新增
#define STIMER_STOP         _IO('s', 3)   // 新增

extern int run_train_once(void);
static struct termios orig_termios;

/* 恢复终端 */
void restore_terminal()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

/* 设置终端为非规范模式 */
void setup_terminal()
{
    struct termios new_termios;

    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(restore_terminal);

    new_termios = orig_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

/* 检查是否有按键 */
int kbhit()
{
    struct timeval tv = {0L, 0L};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
}

int main()
{
    setup_terminal();   

    int fd = open("/dev/stimer", O_RDWR);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    printf("Press any key to exit...\n");

    unsigned int interval = 1000000;

    // 设置周期
    if (ioctl(fd, STIMER_SET_INTERVAL, &interval) < 0) {
        perror("ioctl SET_INTERVAL");
        goto out;
    }

    // 启动定时器（配置硬件并开始运行）
    if (ioctl(fd, STIMER_START) < 0) {
        perror("ioctl START");
        goto out;
    }

    int count = 0;
    int running = 1;

    while (1) {
        if (kbhit()) {
            getchar();   // 读掉输入
            printf("\nKey pressed, exiting...\n");
            // running = 0;
            break;
        }
        char buf[4];
        printf("Waiting for interrupt the %d times...\n", count);

        // 阻塞等待中断
        if (read(fd, buf, sizeof(buf)) < 0) {
            perror("read");
            break;
        }

        printf("Timer interrupt triggered the %d times!\n", count);

        run_train_once();
        count++;
    }

    // 停止定时器
    ioctl(fd, STIMER_STOP);

out:
    close(fd);
    return 0;
}