#define ADDR1 (*(volatile unsigned long long *)0x2680c000ULL)
#define ADDR2 (*(volatile unsigned long long *)0x2680c100ULL)

void _start(void) {
    unsigned long long val;

    // 读 0x2680c000，加 1，写 0x2680c100
    val = ADDR1;
    val++;
    ADDR2 = val;

    // 读 0x2680c100，写 0x2680c000
    val = ADDR2;
    ADDR1 = val;

    // 停机（防止程序跑飞）
    __asm__ volatile ("ebreak" ::: "memory");
    while (1);   // 死循环，保证不会继续执行
}