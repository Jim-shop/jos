/*
处理鼠标初始化、中断
*/

#include "bootpack.h"

/*
为了节省处理资源，鼠标控制电路默认不激活，不发送中断信号。

因为鼠标控制电路默认不激活，
所以鼠标本身干脆默认也不激活。

所以要让鼠标有用，一是要激活鼠标控制电路，二是要激活鼠标本身。

鼠标控制电路包含在键盘控制电路中，
正常完成键盘控制电路的初始化，鼠标电路控制器也就激活了。
*/

void enable_mouse(struct MOUSE_DEC *const mdec)
{
    /*
    激活鼠标本身。
    */
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    // 顺利的话，执行完毕 PORT_KEYDAT 会返回 ACK(0xfa)。
    // 但鼠标反应可能没那么快。
    mdec->phase = 0; // 设置为等待0xfa的阶段
    return;
}

int mouse_decode(struct MOUSE_DEC *const mdec, unsigned char const dat)
{
    /*
        解码鼠标数据。
        返回0表示还未读取完全。
        返回1表示读取成功。
        返回-1表示意外错误。
    */
    switch (mdec->phase)
    {
    case 0:
        // 等待 0xfa
        if (dat == 0xfa)
            mdec->phase = 1;
        return 0;
    case 1:
        // 等待第一字节
        if ((dat & 0xc8) == 0x08)
        { // 如果第一字节正确
            mdec->buf[0] = dat;
            mdec->phase = 2;
        }
        return 0;
    case 2:
        // 等待第二字节
        mdec->buf[1] = dat;
        mdec->phase = 3;
        return 0;
    case 3:
        // 等待第三字节
        mdec->buf[2] = dat;
        mdec->phase = 1;
        mdec->btn = mdec->buf[0] & 0x07; // 取低三位
        mdec->x = mdec->buf[1];
        mdec->y = mdec->buf[2];
        if ((mdec->buf[0] & 0x10) != 0)
            mdec->x |= 0xffffff00; // int类型4个字节，防止读入内存中的垃圾
        if ((mdec->buf[0] & 0x20) != 0)
            mdec->y |= 0xffffff00;
        mdec->y = -mdec->y; // 鼠标与屏幕y方向相反
        return 1;
    default:
        return -1;
    }
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
