/*
写主函数
*/

#include "bootpack.h"

struct BOOTINFO *const binfo = (struct BOOTINFO *)ADR_BOOTINFO;

void make_window8(unsigned char *buf, int xsize, int ysize, char *title);

void Main(void)
{
    // 初始化中断
    init_gdtidt();
    init_pic();
    // 中断初始化完成，开放中断（但除了IRQ2都禁止了）
    io_sti();

    // 时钟PIT初始化
    init_pit();
    // 定时器1
    struct FIFO8 timerfifo1;
    char timerbuf1[8];
    fifo8_init(&timerfifo1, 8, timerbuf1);
    struct TIMER *timer1 = timer_alloc();
    timer_init(timer1, &timerfifo1, 1);
    timer_settime(timer1, 1000);
    // 定时器2
    struct FIFO8 timerfifo2;
    char timerbuf2[8];
    fifo8_init(&timerfifo2, 8, timerbuf2);
    struct TIMER *timer2 = timer_alloc();
    timer_init(timer2, &timerfifo2, 1);
    timer_settime(timer2, 300);
    // 定时器3
    struct FIFO8 timerfifo3;
    char timerbuf3[8];
    fifo8_init(&timerfifo3, 8, timerbuf3);
    struct TIMER *timer3 = timer_alloc();
    timer_init(timer3, &timerfifo3, 1);
    timer_settime(timer3, 50);

    // 键鼠FIFO初始化
    extern struct FIFO8 keyfifo, mousefifo;
    char keybuf[32];
    fifo8_init(&keyfifo, 32, keybuf);
    char mousebuf[128];
    fifo8_init(&mousefifo, 128, mousebuf);
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
    struct SHEET *sht_back = sheet_alloc(shtctl);
    unsigned char *buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->SCRNX * binfo->SCRNY);
    sheet_setbuf(sht_back, buf_back, binfo->SCRNX, binfo->SCRNY, -1); // 没有透明色
    init_screen8(buf_back, binfo->SCRNX, binfo->SCRNY);
    sheet_slide(sht_back, 0, 0); // 此时图层还不可见，不会绘图
    // 窗口图层
    struct SHEET *sht_win = sheet_alloc(shtctl);
    unsigned char *buf_win = (unsigned char *)memman_alloc_4k(memman, 160 * 52);
    sheet_setbuf(sht_win, buf_win, 160, 52, -1); // 没有透明色
    make_window8(buf_win, 160, 52, "counter");
    sheet_slide(sht_win, 80, 72);
    // 鼠标光标图层
    struct SHEET *sht_mouse = sheet_alloc(shtctl);
    unsigned char buf_mouse[256];
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
    init_mouse_cursor8(buf_mouse, 99);
    int mx = (binfo->SCRNX - 16) / 2; //画面中心坐标
    int my = (binfo->SCRNY - 28 - 16) / 2;
    sheet_slide(sht_mouse, mx, my); // 此时图层还不可见，不会绘图
    // 设定图层高度
    sheet_updown(sht_back, 0);
    sheet_updown(sht_win, 1);
    sheet_updown(sht_mouse, 2);

    // 鼠标坐标绘制
    char s[40];
    sprintf(s, "(%3d, %3d)", mx, my);
    putfonts8_asc(buf_back, binfo->SCRNX, 0, 0, white, s);
    // 内存信息显示
    sprintf(s, "memory %dMB free : %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
    putfonts8_asc(buf_back, binfo->SCRNX, 0, 32, white, s);
    sheet_refresh(sht_back, 0, 0, binfo->SCRNX, 48);

    // 开放中断
    io_out8(PIC0_IMR, 0xf8); // 11111000 开放时钟、从PIC、键盘中断
    io_out8(PIC1_IMR, 0xef); // 11101111 开放鼠标中断

    int i;
    for (;;)
    {
        sprintf(s, "%010d", timerctl.count);
        boxfill8(buf_win, 160, gray, 40, 28, 119, 43);
        putfonts8_asc(buf_win, 160, 40, 28, black, s);
        sheet_refresh(sht_win, 40, 28, 120, 44);

        io_cli(); // 先屏蔽中断
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) + fifo8_status(&timerfifo1) + fifo8_status(&timerfifo2) + fifo8_status(&timerfifo3) == 0)
            /*
            此处使用stihlt指令，相当于STI指令后紧跟着HLT指令，
            这两条指令执行之间发生中断的话，中断会在HLT指令之后被受理。
            如果分开用STI和HLT，中间发生中断的话，
            CPU会进入HLT状态不处理中断。
            */
            // io_stihlt(); // 恢复接受中断，待机。当收到中断后就会恢复执行HLT指令之后的指令
            io_sti(); // 回复接收中断。不待机，提高性能
        else
        {
            if (fifo8_status(&keyfifo))
            {
                i = fifo8_get(&keyfifo);
                io_sti(); // 恢复中断
                sprintf(s, "%02X", i);
                boxfill8(buf_back, binfo->SCRNX, lightdarkblue, 0, 16, 15, 31);
                putfonts8_asc(buf_back, binfo->SCRNX, 0, 16, white, s);
                sheet_refresh(sht_back, 0, 16, 16, 32);
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
                    sheet_refresh(sht_back, 32, 16, 32 + 15 * 8, 32);
                    // 鼠标指针移动
                    mx += mdec.x;
                    my += mdec.y;
                    if (mx < 0)
                        mx = 0;
                    if (my < 0)
                        my = 0;
                    if (mx > binfo->SCRNX - 1)
                        mx = binfo->SCRNX - 1;
                    if (my > binfo->SCRNY - 1)
                        my = binfo->SCRNY - 1;
                    sprintf(s, "(%3d, %3d)", mx, my);
                    boxfill8(buf_back, binfo->SCRNX, lightdarkblue, 0, 0, 79, 15);
                    putfonts8_asc(buf_back, binfo->SCRNX, 0, 0, white, s);
                    sheet_refresh(sht_back, 0, 0, 80, 16);
                    sheet_slide(sht_mouse, mx, my);
                }
            }
            else if (fifo8_status(&timerfifo1))
            {
                i = fifo8_get(&timerfifo1);
                io_sti();
                putfonts8_asc(buf_back, binfo->SCRNX, 0, 64, white, "10[sec]");
                sheet_refresh(sht_back, 0, 64, 56, 80);
            }
            else if (fifo8_status(&timerfifo2))
            {
                i = fifo8_get(&timerfifo2);
                io_sti();
                putfonts8_asc(buf_back, binfo->SCRNX, 0, 80, white, "3[sec]");
                sheet_refresh(sht_back, 0, 80, 48, 96);
            }
            else if (fifo8_status(&timerfifo3))
            {
                i = fifo8_get(&timerfifo3);
                io_sti();
                if (i != 0)
                {
                    timer_init(timer3, &timerfifo3, 0);
                    boxfill8(buf_back, binfo->SCRNX, white, 8, 96, 15, 111);
                }
                else
                {
                    timer_init(timer3, &timerfifo3, 1);
                    boxfill8(buf_back, binfo->SCRNX, lightdarkblue, 8, 96, 15, 111);
                }
                timer_settime(timer3, 50);
                sheet_refresh(sht_back, 8,96,16,112);
            }
        }
    }
}

void make_window8(unsigned char *const buf, const int xsize, const int ysize, char *const title)
{
    /*
    制造窗口的buf
    */
    static char closebtn[14][16] =
        {
            "OOOOOOOOOOOOOOO@",
            "OQQQQQQQQQQQQQ$@",
            "OQQQQQQQQQQQQQ$@",
            "OQQQ@@QQQQ@@QQ$@",
            "OQQQQ@@QQ@@QQQ$@",
            "OQQQQQ@@@@QQQQ$@",
            "OQQQQQQ@@QQQQQ$@",
            "OQQQQQ@@@@QQQQ$@",
            "OQQQQ@@QQ@@QQQ$@",
            "OQQQ@@QQQQ@@QQ$@",
            "OQQQQQQQQQQQQQ$@",
            "OQQQQQQQQQQQQQ$@",
            "O$$$$$$$$$$$$$$@",
            "@@@@@@@@@@@@@@@@",
        };
    int x, y;
    char c;
    boxfill8(buf, xsize, gray, 0, 0, xsize - 1, 0);
    boxfill8(buf, xsize, white, 1, 1, xsize - 2, 1);
    boxfill8(buf, xsize, gray, 0, 0, 0, ysize - 1);
    boxfill8(buf, xsize, white, 1, 1, 1, ysize - 2);
    boxfill8(buf, xsize, darkgray, xsize - 2, 1, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, black, xsize - 1, 0, xsize - 1, ysize - 1);
    boxfill8(buf, xsize, gray, 2, 2, xsize - 3, ysize - 3);
    boxfill8(buf, xsize, darkblue, 3, 3, xsize - 4, 20);
    boxfill8(buf, xsize, darkgray, 1, ysize - 2, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, black, 0, ysize - 1, xsize - 1, ysize - 1);
    putfonts8_asc(buf, xsize, 24, 4, white, title);
    for (y = 0; y < 14; y++)
        for (x = 0; x < 16; x++)
        {
            c = closebtn[y][x];
            if (c == '@')
                c = black;
            else if (c == '$')
                c = darkgray;
            else if (c == 'Q')
                c = gray;
            else
                c = white;
            buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
        }
    return;
}
