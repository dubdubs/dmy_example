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
    int fd = open("/dev/stimer", O_RDWR);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    printf("Press any key to exit...\n");

    unsigned int interval = 100000;

    if (ioctl(fd, STIMER_SET_INTERVAL, &interval) < 0) {
        perror("ioctl");
    }

    while (1) {
        if (kbhit()) {
            getchar();   // 读掉输入
            printf("\nKey pressed, exiting...\n");
            break;
        }
        char buf[4];
        printf("Waiting for interrupt...\n");

        /* 阻塞等待中断 */
        read(fd, buf, sizeof(buf));

        printf("Timer interrupt triggered!\n");

        run_train_once();
    }

    close(fd);
    return 0;
}