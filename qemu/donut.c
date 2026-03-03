#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>

// 预定义的颜色序列（ANSI 256色：由深蓝到浅蓝再到白色）
int colors[] = {18, 19, 20, 21, 27, 33, 39, 45, 51, 81, 117, 153, 231};

void draw_donut(int width, int height, float A, float B, int frame) {
    float z[width * height];
    char b[width * height];
    int c[width * height]; // 存储颜色索引
    
    // 初始化缓存
    memset(b, 32, width * height);
    memset(z, 0, sizeof(float) * width * height);
    
    // 环面参数
    // theta 和 phi 的步长决定了渲染精细度
    for (float j = 0; j < 6.28; j += 0.07) {
        for (float i = 0; i < 6.28; i += 0.02) {
            float c_i = cos(i), s_i = sin(i);
            float c_j = cos(j), s_j = sin(j);
            float c_A = cos(A), s_A = sin(A);
            float c_B = cos(B), s_B = sin(B);

            float h = c_j + 2; // 2 是环面的半径
            float D = 1 / (s_i * h * s_A + s_j * c_A + 5); // 5 是距离观察者的距离
            float t = s_i * h * c_A - s_j * s_A;

            // 投影到 2D 坐标
            int x = (int)((width / 2) + (width / 2.5) * D * (c_i * h * c_B - t * s_B));
            int y = (int)((height / 2) + (height / 1.5) * D * (c_i * h * s_B + t * c_B));

            // 计算亮度 (L)，范围约 -8 到 8
            float L_val = 8 * ((s_j * s_A - s_i * c_j * c_A) * c_B - s_i * c_j * s_A - s_j * c_A - c_i * c_j * s_B);
            int L = (int)L_val;

            // 如果坐标在屏幕内且更接近观察者
            if (height > y && y > 0 && x > 0 && width > x && D > z[y * width + x]) {
                z[y * width + x] = D;
                // 亮度字符映射
                b[y * width + x] = ".,-~:;=!*#$@"[L > 0 ? L : 0];
                // 颜色映射：根据亮度选择颜色
                c[y * width + x] = colors[L > 0 ? (L < 12 ? L : 12) : 0];
            }
        }
    }

    // 打印到屏幕
    printf("\033[H"); // 光标回到左上角
    for (int k = 0; k < width * height; k++) {
        if (k % width == 0 && k != 0) putchar('\n');
        if (b[k] != 32) {
            printf("\033[38;5;%dm%c", c[k], b[k]);
        } else {
            putchar(32);
        }
    }
    printf("\033[0m\n");
    printf("\033[1;37m Frame: %d | Res: %dx%d | Status: %s \033[0m\n", 
           frame, width, height, (width > 60) ? "High-Res" : "Standard");
}

int main(int argc, char *argv[]) {
    int width = 80;
    int height = 40;

    if (argc >= 3) {
        width = atoi(argv[1]);
        height = atoi(argv[2]);
    }

    float A = 0, B = 0;
    int frame = 1;

    printf("\033[2J"); // 清屏
    printf("\033[?25l"); // 隐藏光标

    while (1) {
        draw_donut(width, height, A, B, frame);
        
        A += 0.08; // 旋转速度
        B += 0.04;
        frame++;

        // 加延时以方便观看
        // 但在 Pure RTL 上，计算本身就是巨大的延时
        usleep(30000); 
    }

    printf("\033[?25h"); // 恢复光标
    return 0;
}