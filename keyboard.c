/*
处理键盘初始化、中断
*/

#include "bootpack.h"

void wait_KBC_sendready(void)
{
    /*
    等待键盘控制电路(KeyBoard Controller)准备完毕
    */
    for (;;)
        if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0)
            return;
}

void init_keyboard(void)
{
    /*
    初始化键盘控制电路（顺带激活鼠标控制电路）
    */
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE);
    return;
}

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
