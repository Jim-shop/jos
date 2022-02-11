/*
中断
*/

#include "bootpack.h"
extern struct BOOTINFO *const binfo;

/*
可编程中断控制器PIC有两个，主PIC直接连接CPU，处理中断信号IRQ 0~7，
从PIC连接在IRQ 2上，处理IRQ 8~15。
中断发生后，如果CPU可以受理中断，CPU就会要求PIC发送两个字节的数据：
0xcd 0x??。也就是机器指令INT 0x??。
CPU会把送过来的这两个字节当机器指令执行。

PIC寄存器：（都是8位）写入ICW1后一定要紧接着写入ICW2等规则
IMR：中断屏蔽寄存器(interrupt mask register)，8位对应8路IRQ信号，哪位为1屏蔽哪位
ICW：初始化控制数据(initial control word)，有四个。
    ICW2：设定IRQ通知CPU的INT指令后的号码（注意INT 0x00~0x1f供CPU内部使用）
    ICW3：对于主PIC：设置从PIC。为1的位代表1个从PIC的连接位点（可以多个）
          对于从PIC：知会连接主PIC的IRQ号（0~7）
*/

void init_pic(void)
{
    /*
    初始化可编程中断控制器PIC
    */

    //屏蔽中断，防止干扰
    io_out8(PIC0_IMR, 0xff); // 禁止所有中断
    io_out8(PIC1_IMR, 0xff); // 禁止所有中断

    io_out8(PIC0_ICW1, 0x11);   // 边缘触发模式
    io_out8(PIC0_ICW2, 0x20);   // IRQ 0~7 由 INT 20~27 接收
    io_out8(PIC0_ICW3, 1 << 2); // PIC 1 由 IRQ 2连接
    io_out8(PIC0_ICW4, 0x01);   // 无缓冲区模式

    io_out8(PIC1_ICW1, 0x11); // 边缘触发模式
    io_out8(PIC1_ICW2, 0x28); // IRQ 8~15 由 INT 28~2f 接收
    io_out8(PIC1_ICW3, 2);    // PIC 1 由 IRQ 2连接
    io_out8(PIC1_ICW4, 0x01); // 无缓冲区模式

    io_out8(PIC0_IMR, 0xfb); // 11111011 PIC 1 之外全部禁止
    io_out8(PIC1_IMR, 0xff); // 禁止所有中断

    return;
}

/*
    中断处理完成后，使用指令
    io_out8(PIC0_OCW2, 0x61);
    通知PIC IRQ 已经受理完毕，
    这样PIC就能继续时刻监视该IRQ中断是否发生。
    没有通知的话，PIC就不再监视对应IRQ中断。

    PIC0：中断的PIC
    0x61：对于PIC0：0x60+IRQ号(0~7)
          对于PIC1：0x60+IRQ号(8~15) - 8

    PIC1 通知完后还要通知 PIC0 的 IRQ 2，
    不然 PIC1 的中断会被 PIC0 忽略。
*/

struct FIFO8 keyfifo;

void inthandler21(int *esp)
{
    /*
    处理来自PS/2键盘的中断
    */
    io_out8(PIC0_OCW2, 0x61); // IRG 0x01中断受理完成（+0x60）
    fifo8_put(&keyfifo, io_in8(PORT_KEYDAT));
    return;
}

struct FIFO8 mousefifo;

void inthandler2c(int *esp)
{
    /*
    处理来自PS/2鼠标的中断
    */
    io_out8(PIC1_OCW2, 0x64); // PIC1 IRQ 12
    io_out8(PIC0_OCW2, 0x62); // PIC0 IRQ 2
    fifo8_put(&mousefifo, io_in8(PORT_KEYDAT));
    return;
}

void inthandler27(int *esp)
{
    /* PIC0中断的不完整策略
    这个中断在Athlon64X2上通过芯片组提供的便利，只需执行一次
    这个中断只是接收，不执行任何操作
    为什么不处理？
        →  因为这个中断可能是电气噪声引发的、只是处理一些重要的情况。*/
    io_out8(PIC0_OCW2, 0x67); /* 通知PIC的IRQ-07 */
    return;
}
