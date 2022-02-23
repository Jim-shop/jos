/*
处理键盘初始化、中断
*/

#include "bootpack.h"

struct FIFO32 *keyfifo;
int keydata0; // 偏移

void wait_KBC_sendready(void)
{
    /*
    等待键盘控制电路(KeyBoard Controller)准备完毕
    */
    for (;;)
        if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0)
            return;
}

void init_keyboard(struct FIFO32 *const fifo, const int data0)
{
    /*
    初始化FIFO；初始化键盘控制电路（顺带激活鼠标控制电路）
    */
    keyfifo = fifo;
    keydata0 = data0;

    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE);
    return;
}

void inthandler21(int *esp)
{
    /*
    处理来自PS/2键盘的中断
    */
    io_out8(PIC0_OCW2, 0x61); // IRQ 0x01中断受理完成（+0x60）
    fifo32_put(keyfifo, keydata0 + io_in8(PORT_KEYDAT));
    return;
}
