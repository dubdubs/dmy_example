#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

typedef struct { float x, y, z; } Point3D;
typedef struct { int v1, v2; } Edge;

#define MAX_POINTS 100
#define MAX_EDGES 200

Point3D v_buf[MAX_POINTS];
Edge e_buf[MAX_EDGES];
int curr_v_count = 0, curr_e_count = 0;

// --- 1. 3D 十字架 (3D Cross) ---
// 由三个轴向的长条组成，非常容易看出旋转
void load_cross() {
    curr_v_count = 0; curr_e_count = 0;
    float w = 2.0, t = 0.4; // w是长度, t是厚度
    Point3D v[] = {
        {-w,-t,t},{w,-t,t},{w,t,t},{-w,t,t},{-w,-t,-t},{w,-t,-t},{w,t,-t},{-w,t,-t}, // X轴长条
        {-t,-w,t},{t,-w,t},{t,w,t},{-t,w,t},{-t,-w,-t},{t,-w,-t},{t,w,-t},{-t,w,-t}  // Y轴长条
    };
    Edge e[] = {
        {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}, // X轴盒子的边
        {8,9},{9,10},{10,11},{11,8},{12,13},{13,14},{14,15},{15,12},{8,12},{9,13},{10,14},{11,15} // Y轴盒子的边
    };
    memcpy(v_buf, v, sizeof(v)); memcpy(e_buf, e, sizeof(e));
    curr_v_count = 16; curr_e_count = 24;
}

// --- 2. 星形四面体 (Star Tetrahedron) ---
// 看起来像一个 3D 的六角星，棱角极其鲜明
void load_star() {
    curr_v_count = 0; curr_e_count = 0;
    float s = 1.5;
    Point3D v[] = {
        {s,s,s},{s,-s,-s},{-s,s,-s},{-s,-s,s},      // 第一个四面体
        {-s,-s,-s},{-s,s,s},{s,-s,s},{s,s,-s}       // 第二个四面体 (反向)
    };
    Edge e[] = {
        {0,1},{0,2},{0,3},{1,2},{1,3},{2,3},        // 四面体1的6条边
        {4,5},{4,6},{4,7},{5,6},{5,7},{6,7}         // 四面体2的6条边
    };
    memcpy(v_buf, v, sizeof(v)); memcpy(e_buf, e, sizeof(e));
    curr_v_count = 8; curr_e_count = 12;
}

// --- 3. 沙漏 (Hourglass) ---
void load_hourglass() {
    curr_v_count = 0; curr_e_count = 0;
    float w = 1.5, h = 2.0;
    Point3D v[] = {
        {-w,h,w},{w,h,w},{w,h,-w},{-w,h,-w}, // 顶面
        {0,0,0},                             // 中心点
        {-w,-h,w},{w,-h,w},{w,-h,-w},{-w,-h,-w} // 底面
    };
    Edge e[] = {
        {0,1},{1,2},{2,3},{3,0},             // 顶面边
        {0,4},{1,4},{2,4},{3,4},             // 连向中心
        {5,4},{6,4},{7,4},{8,4},             // 连向中心
        {5,6},{6,7},{7,8},{8,5}              // 底面边
    };
    memcpy(v_buf, v, sizeof(v)); memcpy(e_buf, e, sizeof(e));
    curr_v_count = 9; curr_e_count = 16;
}

// --- 渲染引擎 (保持 Bresenham 画线算法不变) ---
void draw_line(char *buf, int w, int h, int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (1) {
        if (x0 >= 0 && x0 < w && y0 >= 0 && y0 < h) buf[y0 * w + x0] = '#';
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

int main(int argc, char *argv[]) {
    int width = 100, height = 40;
    if (argc >= 3) { width = atoi(argv[1]); height = atoi(argv[2]); }
    char *buffer = malloc(width * height);
    float angle = 0;
    int round = 0, shape_idx = 0;
    int frames_per_shape = 100;
    int frame_in_shape = 0;

    char* shape_names[] = {"3D CROSS", "STAR TETRAHEDRON", "HOURGLASS", "CLASSIC CUBE"};
    int colors[] = {36, 35, 33, 34}; // 青, 紫, 黄, 蓝

    printf("\033[2J\033[?25l");

    while (1) {
        if (frame_in_shape == 0) {
            if (shape_idx == 0) load_cross();
            else if (shape_idx == 1) load_star();
            else if (shape_idx == 2) load_hourglass();
            else if (shape_idx == 3) {
                // 加载普通立方体作为对比
                Point3D v[] = {{-1.2,-1.2,1.2},{1.2,-1.2,1.2},{1.2,1.2,1.2},{-1.2,1.2,1.2},{-1.2,-1.2,-1.2},{1.2,-1.2,-1.2},{1.2,1.2,-1.2},{-1.2,1.2,-1.2}};
                Edge e[] = {{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
                memcpy(v_buf, v, sizeof(v)); memcpy(e_buf, e, sizeof(e));
                curr_v_count = 8; curr_e_count = 12;
            }
        }

        memset(buffer, ' ', width * height);
        printf("\033[H\n");

        int px[MAX_POINTS], py[MAX_POINTS];
        for(int i=0; i<curr_v_count; i++) {
            float x = v_buf[i].x, y = v_buf[i].y, z = v_buf[i].z;
            float tx, ty, tz;
            // 旋转
            ty = y*cos(angle) - z*sin(angle); tz = y*sin(angle) + z*cos(angle); y = ty; z = tz;
            tx = x*cos(angle*0.6) + z*sin(angle*0.6); tz = -x*sin(angle*0.6) + z*cos(angle*0.6); x = tx;
            
            float d = 1.0 / (z + 5.0);
            px[i] = (int)(width/2 + x * d * 30.0 * 2.8);
            py[i] = (int)(height/2 + y * d * 30.0);
        }

        printf("\033[1;%dm", colors[shape_idx]);
        for(int i=0; i<curr_e_count; i++) 
            draw_line(buffer, width, height, px[e_buf[i].v1], py[e_buf[i].v1], px[e_buf[i].v2], py[e_buf[i].v2]);

        for(int i=0; i<height; i++) {
            for(int j=0; j<width; j++) putchar(buffer[i*width + j]);
            putchar('\n');
        }

        printf("\033[1;37m ROUND: %d | SHAPE: %s | EDGES: %d \033[0m\n", round, shape_names[shape_idx], curr_e_count);
        printf("\033[1;32m [SPEED MONITOR] Hybrid simulation allows deep software validation today! \033[0m");
        fflush(stdout);

        angle += 0.07;
        frame_in_shape++;
        if (frame_in_shape >= frames_per_shape) {
            frame_in_shape = 0;
            shape_idx++;
            if (shape_idx >= 4) { shape_idx = 0; round++; }
        }
        usleep(30000);
    }
    return 0;
}