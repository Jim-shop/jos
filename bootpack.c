/*
写主函数
*/

#include "bootpack.h"

struct BOOTINFO *const binfo = (struct BOOTINFO *)ADR_BOOTINFO;

void Main(void)
{
    // 初始化中断
    init_gdtidt();
    init_pic();
    // 中断初始化完成，开放中断
    io_sti();

    // 键鼠FIFO初始化
    extern struct FIFO8 keyfifo, mousefifo;
    char keybuf[32];
    fifo8_init(&keyfifo, 32, keybuf);
    char mousebuf[128];
    fifo8_init(&mousefifo, 128, mousebuf);
    // 键鼠中断初始化
    io_out8(PIC0_IMR, 0xf9); // 11111001 开放PIC1和键盘中断
    io_out8(PIC1_IMR, 0xef); // 11101111 开放鼠标中断
    // 键盘控制器初始化
    init_keyboard();
    // 鼠标本体初始化
    struct MOUSE_DEC mdec;
    enable_mouse(&mdec);

    // 内存管理初始化
    unsigned int memtotal = memtest(0x00400000, 0xbfffffff); //总内存
    struct MEMMAN *const memman = (struct MEMMAN *const)MEMMAN_ADDR;
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000);
    memman_free(memman, 0x00400000, memtotal - 0x00400000);

    // 初始化调色板
    init_palette();
    // 图层管理器
    struct SHTCTL *shtctl = shtctl_init(memman, binfo->VRAM, binfo->SCRNX, binfo->SCRNY);
    // 背景图层
    struct SHEET *sht_back = sheet_alloc(shtctl); // 背景
    unsigned char *buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->SCRNX * binfo->SCRNY);
    sheet_setbuf(sht_back, buf_back, binfo->SCRNX, binfo->SCRNY, -1);
    init_screen8(buf_back, binfo->SCRNX, binfo->SCRNY);
    sheet_slide(shtctl, sht_back, 0, 0); // 此时图层还不可见，不会绘图
    // 鼠标光标图层
    struct SHEET *sht_mouse = sheet_alloc(shtctl); // 鼠标光标
    unsigned char buf_mouse[256];
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
    init_mouse_cursor8(buf_mouse, 99);
    int mx = (binfo->SCRNX - 16) / 2; //画面中心坐标
    int my = (binfo->SCRNY - 28 - 16) / 2;
    sheet_slide(shtctl, sht_mouse, mx, my); // 此时图层还不可见，不会绘图
    // 设定图层高度
    sheet_updown(shtctl, sht_back, 0);
    sheet_updown(shtctl, sht_mouse, 1);
    // putblock8_8(binfo->VRAM, binfo->SCRNX, 16, 16, mx, my, mcursor, 16);

    // 鼠标坐标绘制
    char s[40];
    sprintf(s, "(%3d, %3d)", mx, my);
    putfonts8_asc(buf_back, binfo->SCRNX, 0, 0, white, s);
    // 内存信息显示
    sprintf(s, "memory %dMB free : %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
    putfonts8_asc(buf_back, binfo->SCRNX, 0, 32, white, s);
    sheet_refresh(shtctl, sht_back, 0, 0, binfo->SCRNX, 48);

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
                boxfill8(buf_back, binfo->SCRNX, lightdarkblue, 0, 16, 15, 31);
                putfonts8_asc(buf_back, binfo->SCRNX, 0, 16, white, s);
                sheet_refresh(shtctl, sht_back, 0, 16, 16, 32);
            }
            else if (fifo8_status(&mousefifo))
            {
                i = fifo8_get(&mousefifo);
                io_sti();
                if (mouse_decode(&mdec, i))
                {
                    // 鼠标按键信息绘制
                    sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
                    if (mdec.btn & 0x01)
                        s[1] = 'L';
                    if (mdec.btn & 0x02)
                        s[3] = 'R';
                    if (mdec.btn & 0x04)
                        s[2] = 'C';
                    boxfill8(buf_back, binfo->SCRNX, lightdarkblue, 32, 16, 32 + 15 * 8 - 1, 31);
                    putfonts8_asc(buf_back, binfo->SCRNX, 32, 16, white, s);
                    sheet_refresh(shtctl, sht_back, 32, 16, 32+15*8, 32);
                    // 鼠标指针移动
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
                    boxfill8(buf_back, binfo->SCRNX, lightdarkblue, 0, 0, 79, 15);
                    putfonts8_asc(buf_back, binfo->SCRNX, 0, 0, white, s);
                    sheet_refresh(shtctl, sht_back, 0, 0, 80, 16);
                    sheet_slide(shtctl, sht_mouse, mx, my);
                }
            }
        }
    }
}
