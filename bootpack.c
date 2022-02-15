/*
写主函数
*/

#include "bootpack.h"

struct BOOTINFO const *const binfo = (struct BOOTINFO const *const)ADR_BOOTINFO;

void make_window8(unsigned char *const buf, const int xsize, const int ysize, char *const title, char const act);
void make_textbox8(struct SHEET *const sht, const int x0, const int y0, const int sx, const int sy, const int c);
void putfonts8_asc_sht(struct SHEET *const sht, const int x, const int y, const int c, const int b, char const *const s, const int l);

// 可显示键位表
const static char keytable[0x54] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
    0, 0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']',
    0, 0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`',
    0, '\\', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.'};

// 各个主函数不要使用return语句，return返回[ESP]地址，而[ESP]这里没有写入

void task_b_main(struct SHEET *const sht_win_b);

void Main(void)
{
    // 各种用途的临时变量
    int i;
    char s[40];

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
    fifo32_init(&fifo, 128, fifobuf, 0);

    // 时钟PIT初始化
    init_pit();
    // 光标闪烁定时器
    struct TIMER *timer_blink = timer_alloc();
    timer_init(timer_blink, &fifo, 1);
    timer_settime(timer_blink, 50);

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

    // 多任务初始化，顺便标记当前任务
    struct TASK *task_a = task_init(memman);
    // FIFO有变的时候解除休眠
    fifo.task = task_a;
    // 改变A的优先级（不需改变的话不需taskrun）
    task_run(task_a, 1, 0);
    // task_b_main
    struct TASK *task_b[3];

    // 初始化调色板
    init_palette();
    // 图层管理器
    struct SHTCTL *shtctl = shtctl_init(memman, binfo->VRAM, binfo->SCRNX, binfo->SCRNY);
    // 鼠标光标图层
    struct SHEET *sht_mouse = sheet_alloc(shtctl);
    unsigned char buf_mouse[256];
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
    init_mouse_cursor8(buf_mouse, 99);
    int mx = (binfo->SCRNX - 16) / 2; // 光标位置，取画面中心坐标
    int my = (binfo->SCRNY - 28 - 16) / 2;
    // 背景图层
    struct SHEET *sht_back = sheet_alloc(shtctl);
    unsigned char *buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->SCRNX * binfo->SCRNY);
    sheet_setbuf(sht_back, buf_back, binfo->SCRNX, binfo->SCRNY, -1); // 没有透明色
    init_screen8(buf_back, binfo->SCRNX, binfo->SCRNY);
    // 窗口A图层
    struct SHEET *sht_win = sheet_alloc(shtctl);
    unsigned char *buf_win = (unsigned char *)memman_alloc_4k(memman, 160 * 52);
    sheet_setbuf(sht_win, buf_win, 160, 52, -1); // 没有透明色
    make_window8(buf_win, 160, 52, "task_a", 1);
    make_textbox8(sht_win, 8, 28, 144, 16, white);
    int cursor_x = 8;     // 光标位置
    int cursor_c = white; // 光标颜色
    // 窗口B[0,1,2]图层
    struct SHEET *sht_win_b[3];
    unsigned char *buf_win_b;
    for (i = 0; i < 3; i++)
    {
        sht_win_b[i] = sheet_alloc(shtctl);
        buf_win_b = (unsigned char *)memman_alloc_4k(memman, 144 * 52);
        sheet_setbuf(sht_win_b[i], buf_win_b, 144, 52, -1); //无透明色
        sprintf(s, "task_b%d", i);
        make_window8(buf_win_b, 144, 52, s, 0);
        task_b[i] = task_alloc();
        // 申请64k栈并指向栈底，-8方便我们放一个位于ESP+4的4字节C语言参数
        task_b[i]->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;
        // 将sht_back地址作为B的第一个参数
        *((int *)(task_b[i]->tss.esp + 4)) = (int)sht_win_b[i];
        task_b[i]->tss.eip = (int)&task_b_main;
        task_b[i]->tss.es = 1 * 8;
        task_b[i]->tss.cs = 2 * 8; // GDT 2号
        task_b[i]->tss.ss = 1 * 8;
        task_b[i]->tss.ds = 1 * 8;
        task_b[i]->tss.fs = 1 * 8;
        task_b[i]->tss.gs = 1 * 8;
        task_run(task_b[i], 2, i + 1);
    }
    // 设定图层位置
    sheet_slide(sht_back, 0, 0);    // 此时图层还不可见，不会绘图
    sheet_slide(sht_mouse, mx, my); // 此时图层还不可见，不会绘图
    sheet_slide(sht_win, 8, 56);
    sheet_slide(sht_win_b[0], 168, 56);
    sheet_slide(sht_win_b[1], 8, 116);
    sheet_slide(sht_win_b[2], 168, 116);
    // 设定图层高度
    sheet_updown(sht_back, 0);
    sheet_updown(sht_win_b[0], 1);
    sheet_updown(sht_win_b[1], 2);
    sheet_updown(sht_win_b[2], 3);
    sheet_updown(sht_win, 4);
    sheet_updown(sht_mouse, 5);
    // 鼠标坐标绘制
    sprintf(s, "(%3d, %3d)", mx, my);
    putfonts8_asc_sht(sht_back, 0, 0, white, lightdarkblue, s, 10);
    // 内存信息显示
    sprintf(s, "memory %dMB   free : %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
    putfonts8_asc_sht(sht_back, 0, 32, white, lightdarkblue, s, 40);

    // 开放中断
    io_out8(PIC0_IMR, 0xf8); // 11111000 开放时钟、从PIC、键盘中断
    io_out8(PIC1_IMR, 0xef); // 11101111 开放鼠标中断

    for (;;)
    {
        io_cli(); // 先屏蔽中断
        if (fifo32_status(&fifo) == 0)
        {
            // io_stihlt(); // 恢复接受中断，待机。当收到中断后就会恢复执行HLT指令之后的指令
            task_sleep(task_a); // 线程休眠
            io_sti();           // 由于线程休眠函数中没有屏蔽中断，所以sti要在其之后，不然sleep可能出问题
        }
        else
        {
            i = fifo32_get(&fifo);
            io_sti();                 // 取得消息种类后就可以恢复中断
            if (256 <= i && i <= 511) // 键盘
            {
                sprintf(s, "%02X", i - 256);
                putfonts8_asc_sht(sht_back, 0, 16, white, lightdarkblue, s, 2);
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
                    if (mdec.btn & 0x01)
                        sheet_slide(sht_win, mx - 80, my - 8);
                }
            }
            else if (i <= 1) // 键盘光标定时器
            {
                if (i != 0)
                {
                    timer_init(timer_blink, &fifo, 0);
                    cursor_c = black;
                }
                else
                {
                    timer_init(timer_blink, &fifo, 1);
                    cursor_c = white;
                }
                timer_settime(timer_blink, 50);
                boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
            }
        }
    }
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

void make_window8(unsigned char *const buf, const int xsize, const int ysize, char *const title, char const act)
{
    /*
    制造窗口的buf
    */
    const static char closebtn[14][16] =
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
    char c, tc, tbc; // tc: 标题颜色 tbc：标题栏背景颜色
    if (act != 0)
    {
        tc = white;
        tbc = darkblue;
    }
    else
    {
        tc = gray;
        tbc = darkgray;
    }
    boxfill8(buf, xsize, gray, 0, 0, xsize - 1, 0);
    boxfill8(buf, xsize, white, 1, 1, xsize - 2, 1);
    boxfill8(buf, xsize, gray, 0, 0, 0, ysize - 1);
    boxfill8(buf, xsize, white, 1, 1, 1, ysize - 2);
    boxfill8(buf, xsize, darkgray, xsize - 2, 1, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, black, xsize - 1, 0, xsize - 1, ysize - 1);
    boxfill8(buf, xsize, gray, 2, 2, xsize - 3, ysize - 3);
    boxfill8(buf, xsize, tbc, 3, 3, xsize - 4, 20);
    boxfill8(buf, xsize, darkgray, 1, ysize - 2, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, black, 0, ysize - 1, xsize - 1, ysize - 1);
    putfonts8_asc(buf, xsize, 24, 4, tc, title);
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

void task_b_main(struct SHEET *const sht_win_b)
{
    // 事件队列
    struct FIFO32 fifo;
    int fifobuf[128];
    fifo32_init(&fifo, 128, fifobuf, 0);

    // 性能测试
    struct TIMER *timer_1s = timer_alloc();
    timer_init(timer_1s, &fifo, 100);
    timer_settime(timer_1s, 100);

    int count = 0, count0 = 0;
    char s[12];

    int i;
    for (;;)
    {
        count++;

        io_cli();
        if (fifo32_status(&fifo) == 0)
        {
            io_sti(); // 火力全开
        }
        else
        {
            i = fifo32_get(&fifo);
            io_sti();
            if (i == 100)
            {
                sprintf(s, "%11d", count - count0);
                putfonts8_asc_sht(sht_win_b, 24, 28, black, gray, s, 11);
                count0 = count;
                timer_settime(timer_1s, 100);
            }
        }
    }
}
