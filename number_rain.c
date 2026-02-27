#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

// 256 色模式下的绿色渐变序列 (从深到亮)
// 22: 深绿, 28: 中绿, 34: 亮绿, 40: 翠绿, 46: 极亮绿, 231: 白色
int green_shades[] = {22, 28, 34, 40, 46, 231};

void clear_screen() {
    printf("\033[2J");
}

int main(int argc, char *argv[]) {
    int width = 80, height = 40;
    if (argc >= 3) {
        width = atoi(argv[1]);
        height = atoi(argv[2]);
    }

    // drops 数组记录每一列“雨头”的位置
    int *drops = malloc(width * sizeof(int));
    for (int i = 0; i < width; i++) drops[i] = rand() % height;

    clear_screen();
    printf("\033[?25l"); // 隐藏光标

    while (1) {
        // 我们不执行全屏清屏，而是直接覆盖，这样闪烁感更小
        for (int i = 0; i < width; i++) {
            // 1. 清除这一列老掉的尾巴（避免留下残影）
            // 假设雨的长度是 15
            int tail = (drops[i] - 15 + height) % height;
            printf("\033[%d;%dH ", tail + 1, i + 1);

            // 2. 绘制雨的身体（渐变效果）
            // 我们从尾巴到头绘制，颜色越来越亮
            for (int len = 0; len < 15; len++) {
                int pos = (drops[i] - len + height) % height;
                int color;
                
                if (len == 0) color = 231;      // 头部是最亮的白色
                else if (len < 3) color = 46;   // 靠近头部的是极亮绿
                else if (len < 6) color = 40;   // 中部亮绿
                else if (len < 9) color = 34;   // 中下部
                else if (len < 12) color = 28;  // 尾部暗绿
                else color = 22;                // 最末尾深绿

                printf("\033[%d;%dH\033[38;5;%dm%c", 
                        pos + 1, i + 1, color, '!' + (rand() % 90));
            }

            // 3. 更新雨头位置
            if (rand() % 10 > 1) { // 增加下落概率
                drops[i] = (drops[i] + 1) % height;
            }
        }

        // 底部显示统计信息（用于对比 Hybrid 速度）
        static int frame_count = 0;
        printf("\033[%d;1H\033[1;37m[ PERFORMANCE ] Frames Rendered: %d | Platform: %s \033[0m", 
               height + 1, frame_count++, (width > 40) ? "RISC-V Hybrid" : "RTL Simulation");

        fflush(stdout);
        
        // 控制 Hybrid 的速度。30ms 左右比较像电影效果
        // 而 RTL 下，计算和打印这几百个转义符的时间早就超过 30ms 了
        usleep(30000); 
    }

    free(drops);
    return 0;
}