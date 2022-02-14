/*
写主函数
*/

#include "bootpack.h"

struct BOOTINFO *const binfo = (struct BOOTINFO *)ADR_BOOTINFO;

void make_window8(unsigned char *buf, int xsize, int ysize, char *title);
void putfonts8_asc_sht(struct SHEET *const sht, const int x, const int y, const int c, const int b, char const *const s, const int l);
void make_textbox8(struct SHEET *const sht, const int x0, const int y0, const int sx, const int sy, const int c);

void Main(void)
{
    // 初始化中断
    init_gdtidt();
    init_pic();
    // 中断初始化完成，开放中断（但除了IRQ2都禁止了）
    io_sti();

    // 事件FIFO
    /*
        0~1     光标闪烁定时器
        3       3秒定时器
        10      10秒定时器
        256~511 键盘输入（键盘 keydata0 := 256）
        512~767 鼠标输入
    */
    struct FIFO32 fifo;
    int fifobuf[128];
    fifo32_init(&fifo, 128, fifobuf);

    // 时钟PIT初始化
    init_pit();
    // 定时器1
    struct TIMER *timer1 = timer_alloc();
    timer_init(timer1, &fifo, 10);
    timer_settime(timer1, 1000);
    // 定时器2
    struct TIMER *timer2 = timer_alloc();
    timer_init(timer2, &fifo, 3);
    timer_settime(timer2, 300);
    // 定时器3
    struct TIMER *timer3 = timer_alloc();
    timer_init(timer3, &fifo, 1);
    timer_settime(timer3, 50);

    // 键盘FIFO、控制器初始化
    init_keyboard(&fifo, 256);
    // 鼠标FIFO、本体初始化
    struct MOUSE_DEC mdec;
    enable_mouse(&fifo, 512, &mdec);

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
    make_window8(buf_win, 160, 52, "window");
    sheet_slide(sht_win, 80, 72);
    // 文本框
    make_textbox8(sht_win, 8, 28, 144, 16, white);
    int cursor_x = 8;     // 光标位置
    int cursor_c = white; // 光标颜色
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
        io_cli(); // 先屏蔽中断
        if (fifo32_status(&fifo) == 0)
            /*
            此处使用stihlt指令，相当于STI指令后紧跟着HLT指令，
            这两条指令执行之间发生中断的话，中断会在HLT指令之后被受理。
            如果分开用STI和HLT，中间发生中断的话，
            CPU会进入HLT状态不处理中断。
            */
            io_stihlt(); // 恢复接受中断，待机。当收到中断后就会恢复执行HLT指令之后的指令
        // io_sti(); // 回复接收中断。不待机，全速运行
        else
        {
            i = fifo32_get(&fifo);
            io_sti();                 // 取得消息种类后就可以恢复中断
            if (256 <= i && i <= 511) // 键盘
            {
                sprintf(s, "%02X", i - 256);
                putfonts8_asc_sht(sht_back, 0, 16, white, lightdarkblue, s, 2);
                static char keytable[0x54] = {
                    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0, 0,
                    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0, 0, 'A', 'S',
                    'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0, 0, ']', 'Z', 'X', 'C', 'V',
                    'B', 'N', 'M', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
                    '2', '3', '0', '.'};
                if (i < 256 + 0x54) // 别越界
                    if (keytable[i - 256] && cursor_x < 144)
                    {
                        s[0] = keytable[i - 256];
                        s[1] = 0;
                        putfonts8_asc_sht(sht_win, cursor_x, 28, black, white, s, 1);
                        cursor_x += 8;
                    }
                if (i == 256 + 0x0e && cursor_x > 8) // 退格键
                {
                    putfonts8_asc_sht(sht_win, cursor_x, 28, black, white, " ", 1);
                    cursor_x -= 8;
                }
                // 光标显示
                boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
            }
            else if (512 <= i && i <= 767) // 鼠标
            {
                if (mouse_decode(&mdec, i - 512))
                {
                    // 鼠标按键信息绘制
                    sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
                    if (mdec.btn & 0x01)
                        s[1] = 'L';
                    if (mdec.btn & 0x02)
                        s[3] = 'R';
                    if (mdec.btn & 0x04)
                        s[2] = 'C';
                    putfonts8_asc_sht(sht_back, 32, 16, white, lightdarkblue, s, 15);
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
                    putfonts8_asc_sht(sht_back, 0, 0, white, lightdarkblue, s, 10);
                    sheet_slide(sht_mouse, mx, my);
                    if(mdec.btn&0x01)
                        sheet_slide(sht_win, mx - 80, my - 8);
                }
            }
            else if (i == 10) // 10秒定时器
            {
                putfonts8_asc_sht(sht_back, 0, 64, white, lightdarkblue, "10[sec]", 7);
            }
            else if (i == 3) // 3秒定时器
            {
                putfonts8_asc_sht(sht_back, 0, 80, white, lightdarkblue, "3[sec]", 6);
            }
            else if (i <= 1) // 键盘光标定时器
            {
                if (i)
                {
                    timer_init(timer3, &fifo, 0);
                    cursor_c = black;
                }
                else
                {
                    timer_init(timer3, &fifo, 1);
                    cursor_c = white;
                }
                boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                timer_settime(timer3, 50);
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
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

void putfonts8_asc_sht(struct SHEET *const sht, const int x, const int y, const int c, const int b, char const *const s, const int l)
{
    /*
    在一个图层的某个位置打印字符串。
    x,y: 坐标   c: 颜色   b: 背景颜色   s: 字串   l: 字串长度
    */
    boxfill8(sht->buf, sht->bxsize, b, x, y, x + l * 8 - 1, y + 15); // 不超尾
    putfonts8_asc(sht->buf, sht->bxsize, x, y, c, s);
    sheet_refresh(sht, x, y, x + l * 8, y + 16); // 超尾
    return;
}

void make_textbox8(struct SHEET *const sht, const int x0, const int y0, const int sx, const int sy, const int c)
{
    /*
    在指定图层画一个背景色为c的文本框。
    */
    int x1 = x0 + sx, y1 = y0 + sy;
    boxfill8(sht->buf, sht->bxsize, darkgray, x0 - 2, y0 - 3, x1 + 1, y0 - 3);
    boxfill8(sht->buf, sht->bxsize, darkgray, x0 - 3, y0 - 3, x0 - 3, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, white, x0 - 3, y1 + 2, x1 + 1, y1 + 2);
    boxfill8(sht->buf, sht->bxsize, white, x1 + 2, y0 - 3, x1 + 2, y1 + 2);
    boxfill8(sht->buf, sht->bxsize, black, x0 - 1, y0 - 2, x1 + 0, y0 - 2);
    boxfill8(sht->buf, sht->bxsize, black, x0 - 2, y0 - 2, x0 - 2, y1 + 0);
    boxfill8(sht->buf, sht->bxsize, gray, x0 - 2, y1 + 1, x1 + 0, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, gray, x1 + 1, y0 - 2, x1 + 1, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, c, x0 - 1, y0 - 1, x1 + 0, y1 + 0);
    return;
}
