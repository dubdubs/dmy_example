.section .text
.globl _start
_start:
    lui     t0, 0x2680c          # t0 = 0x2680c000
    addi    t0, t0, 0x100        # t0 = 0x2680c100
    lui     t1, 0x2680c          # t1 = 0x2680c000
loop:
    ld      a0, 0(t1)            # 从 0x2680c000 读取
    addi    a0, a0, 1
    sd      a0, 0(t0)            # 写入 0x2680c100
    ld      a1, 0(t0)            # 从 0x2680c100 读取
    sd      a1, 0(t1)            # 写入 0x2680c000
    j       loop                  # 无限循环