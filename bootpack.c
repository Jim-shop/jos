/*
写主函数
*/

#include "bootpack.h"

struct BOOTINFO *const binfo = (struct BOOTINFO *)ADR_BOOTINFO;
extern struct FIFO8 keyfifo, mousefifo;
void init_keyboard(void);
void enable_mouse(void);

void HariMain(void)
{
    // 初始化中断
    init_gdtidt();
    init_pic();
    // 中断初始化完成，开放中断
    io_sti();

    // 键盘鼠标FIFO初始化
    char keybuf[32];
    fifo8_init(&keyfifo, 32, keybuf);
    char mousebuf[128];
    fifo8_init(&mousefifo, 128, mousebuf);

    io_out8(PIC0_IMR, 0xf9); // 11111001 开放PIC1和键盘中断
    io_out8(PIC1_IMR, 0xef); // 11101111 开放鼠标中断

    init_keyboard();

    // 初始化界面
    init_palette();
    init_screen8(binfo->VRAM, binfo->SCRNX, binfo->SCRNY);

    char mcursor[256];
    init_mouse_cursor8(mcursor, lightdarkblue);
    int mx = (binfo->SCRNX - 16) / 2;
    int my = (binfo->SCRNY - 28 - 16) / 2;
    putblock8_8(binfo->VRAM, binfo->SCRNX, 16, 16, mx, my, mcursor, 16);

    char s[40];
    sprintf(s, "(%d, %d)", mx, my);
    putfonts8_asc(binfo->VRAM, binfo->SCRNX, 0, 0, white, s);

    enable_mouse();

    int i;
    for (;;)
    {
        io_cli(); // 先屏蔽中断
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0)
            /*
            此处使用stihlt指令，相当于STI指令后紧跟着HLT指令，
            这两条指令执行之间发生中断的话，中断会在HLT指令之后被受理。
            如果分开用sti和HLT，中间发生中断的话，
            CPU会进入HLT状态不处理中断。
            */
            io_stihlt(); // 恢复接受中断，待机
        else
        {
            if (fifo8_status(&keyfifo) != 0)
            {
                i = fifo8_get(&keyfifo);
                io_sti(); // 恢复中断
                sprintf(s, "%02X", i);
                boxfill8(binfo->VRAM, binfo->SCRNX, lightdarkblue, 0, 16, 15, 31);
                putfonts8_asc(binfo->VRAM, binfo->SCRNX, 0, 16, white, s);
            }
            else if (fifo8_status(&mousefifo)!=0)
            {
                i = fifo8_get(&mousefifo);
                io_sti();
                sprintf(s, "%02X", i);
                boxfill8(binfo->VRAM, binfo->SCRNX, lightdarkblue, 32, 16, 47, 31);
                putfonts8_asc(binfo->VRAM, binfo->SCRNX, 32, 16, white, s);
            }
        }
    }
}

#define PORT_KEYDAT 0x0060        // 键盘输入的数据（编码）
#define PORT_KEYSTA 0x0064        // 键盘控制电路的状态输出设备号
#define PORT_KEYCMD 0x0064        // 键盘控制电路的指令设置设备号
#define KEYSTA_SEND_NOTREADY 0x02 // KBC状态输出数据的掩码，得到倒数第二位的值
#define KEYCMD_WRITE_MODE 0x60    // 进入设定模式的指令
#define KBC_MODE 0x47             // 鼠标模式

/*
为了节省处理资源，鼠标控制电路默认不激活，不发送中断信号。

因为鼠标控制电路默认不激活，
所以鼠标本身干脆默认也不激活。

所以要让鼠标有用，一是要激活鼠标控制电路，而是要激活鼠标本身。

鼠标控制电路包含在键盘控制电路中，
正常完成键盘控制电路的初始化，鼠标电路控制器也就激活了。
*/

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

#define KEYCMD_SENDTO_MOUSE 0xd4 // 向键盘控制电路发送这个数据，写往DAT的下一个数据就会自动转发给鼠标
#define MOUSECMD_ENABLE 0xf4     // 激活鼠标指令

void enable_mouse(void)
{
    /*
    激活鼠标本身。
    执行完毕 PORT_KEYDAT 会返回 ACK(0xfa)。
    */
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    return;
}
