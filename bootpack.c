/*
写主函数
*/

#include "bootpack.h"

struct BOOTINFO *const binfo = (struct BOOTINFO *)ADR_BOOTINFO;
extern struct FIFO8 keyfifo, mousefifo;

struct MOUSE_DEC
{
    /*
    除了初始化时产生的0xfa，鼠标发送的数据是3字节1组
    */
    // phase: 0等待0xfa, 1,2,3表示等待第几字节
    unsigned char buf[3], phase;
    int x, y, btn;
};

void init_keyboard(void);
void enable_mouse(struct MOUSE_DEC *const mdec);
int mouse_decode(struct MOUSE_DEC *const mdec, unsigned char const dat);

void HariMain(void)
{
    // 初始化中断
    init_gdtidt();
    init_pic();
    // 中断初始化完成，开放中断
    io_sti();

    // 键鼠FIFO初始化
    char keybuf[32];
    fifo8_init(&keyfifo, 32, keybuf);
    char mousebuf[128];
    fifo8_init(&mousefifo, 128, mousebuf);
    // 键鼠中断初始化
    io_out8(PIC0_IMR, 0xf9); // 11111001 开放PIC1和键盘中断
    io_out8(PIC1_IMR, 0xef); // 11101111 开放鼠标中断
    // 键盘控制器初始化
    init_keyboard(); // 鼠标初始化比较久，放在后面进行

    // 初始化界面
    init_palette();
    init_screen8(binfo->VRAM, binfo->SCRNX, binfo->SCRNY);
    char mcursor[256];
    init_mouse_cursor8(mcursor, lightdarkblue);
    int mx = (binfo->SCRNX - 16) / 2; //画面中心坐标
    int my = (binfo->SCRNY - 28 - 16) / 2;
    putblock8_8(binfo->VRAM, binfo->SCRNX, 16, 16, mx, my, mcursor, 16);
    char s[40];
    sprintf(s, "(%3d, %3d)", mx, my);
    putfonts8_asc(binfo->VRAM, binfo->SCRNX, 0, 0, white, s);

    struct MOUSE_DEC mdec;
    enable_mouse(&mdec);

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
            if (fifo8_status(&keyfifo))
            {
                i = fifo8_get(&keyfifo);
                io_sti(); // 恢复中断
                sprintf(s, "%02X", i);
                boxfill8(binfo->VRAM, binfo->SCRNX, lightdarkblue, 0, 16, 15, 31);
                putfonts8_asc(binfo->VRAM, binfo->SCRNX, 0, 16, white, s);
            }
            else if (fifo8_status(&mousefifo))
            {
                i = fifo8_get(&mousefifo);
                io_sti();
                if (mouse_decode(&mdec, i))
                {
                    sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
                    if ((mdec.btn & 0x01) != 0)
                    {
                        s[1] = 'L';
                    }
                    if ((mdec.btn & 0x02) != 0)
                    {
                        s[3] = 'R';
                    }
                    if ((mdec.btn & 0x04) != 0)
                    {
                        s[2] = 'C';
                    }
                    boxfill8(binfo->VRAM, binfo->SCRNX, lightdarkblue, 32, 16, 32 + 15 * 8 - 1, 31);
                    putfonts8_asc(binfo->VRAM, binfo->SCRNX, 32, 16, white, s);
                    // 鼠标指针移动
                    boxfill8(binfo->VRAM, binfo->SCRNX, lightdarkblue, mx, my, mx + 15, my + 15);
                    mx += mdec.x;
                    my += mdec.y;
                    if (mx < 0)
                        mx = 0;
                    if (my < 0)
                        my = 0;
                    if (mx > binfo->SCRNX - 16)
                        mx = binfo->SCRNX - 16;
                    if (my > binfo->SCRNY - 16)
                        my = binfo->SCRNY - 16;
                    sprintf(s, "(%3d, %3d)", mx, my);
                    boxfill8(binfo->VRAM, binfo->SCRNX, lightdarkblue, 0, 0, 79, 15);
                    putfonts8_asc(binfo->VRAM, binfo->SCRNX, 0, 0, white, s);
                    putblock8_8(binfo->VRAM, binfo->SCRNX, 16, 16, mx, my, mcursor, 16);
                }
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
