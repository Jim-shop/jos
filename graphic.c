/*
界面绘制
*/

#include "bootpack.h"

void init_palette(void)
{
    static unsigned char table_rgb[16 * 3] =
        {
            0x00, 0x00, 0x00, //  0: 黑
            0xff, 0x00, 0x00, //  1: 亮红
            0x00, 0xff, 0x00, //  2: 亮绿
            0xff, 0xff, 0x00, //  3: 亮黄
            0x00, 0x00, 0xff, //  4: 亮蓝
            0xff, 0x00, 0xff, //  5: 亮紫
            0x00, 0xff, 0xff, //  6: 浅亮蓝
            0xff, 0xff, 0xff, //  7: 白
            0xc6, 0xc6, 0xc6, //  8: 亮灰
            0x84, 0x00, 0x00, //  9: 暗红
            0x00, 0x84, 0x00, // 10: 暗绿
            0x84, 0x84, 0x00, // 11: 暗黄
            0x00, 0x00, 0x84, // 12: 暗青
            0x84, 0x00, 0x84, // 13: 暗紫
            0x00, 0x84, 0x84, // 14: 浅暗蓝
            0x84, 0x84, 0x84  // 15: 暗灰
        };
    set_palette(0, 15, table_rgb);
    return;
}

void set_palette(int start, const int end, unsigned char *rgb)
{
    /*
    调色板访问步骤：
        1. 在一连串的访问中屏蔽中断
        2. 将想设定的调色板号码写入0x03c8
        3. 紧接着按RGB顺序写入0x03c9
        4. 如果还想继续设定下一个调色板，
           则省略调色板号码，同样按RGB顺序写入0x03c9即可
        5. 恢复中断
    */
    int eflags = io_load_eflags(); // 记录中断许可标志的值
    io_cli();                      // 将中断许可标志置0，禁止中断
    io_out8(0x03c8, start);
    for (; start <= end; start++)
    {
        io_out8(0x03c9, rgb[0] / 4);
        io_out8(0x03c9, rgb[1] / 4);
        io_out8(0x03c9, rgb[2] / 4);
        rgb += 3;
    }
    io_store_eflags(eflags); // 恢复中断许可标志
    return;
}

void boxfill8(unsigned char *const vram, const int xsize, unsigned char const c, int const x0, int y0, int const x1, int const y1)
{
    /*
    给一个方框填充颜色。(x0,y0)左上 (x1,y1)右下
    */
    int x;
    for (; y0 <= y1; y0++)
        for (x = x0; x <= x1; x++)
            vram[y0 * xsize + x] = c;
    return;
}

void init_screen8(unsigned char *const vram, int const x, int const y)
{
    /*
    画最初的界面
    */

    // 屏幕背景和任务栏
    boxfill8(vram, x, lightdarkblue, 0, 0, x - 1, y - 29);
    boxfill8(vram, x, gray, 0, y - 28, x - 1, y - 28);
    boxfill8(vram, x, white, 0, y - 27, x - 1, y - 27);
    boxfill8(vram, x, gray, 0, y - 26, x - 1, y - 1);
    // 开始按钮
    boxfill8(vram, x, white, 3, y - 24, 59, y - 24);
    boxfill8(vram, x, white, 2, y - 24, 2, y - 4);
    boxfill8(vram, x, darkgray, 3, y - 4, 59, y - 4);
    boxfill8(vram, x, darkgray, 59, y - 23, 59, y - 5);
    boxfill8(vram, x, black, 2, y - 3, 59, y - 3);
    boxfill8(vram, x, black, 60, y - 24, 60, y - 3);
    putfonts8_asc(vram, x, 17, y - 23, white, "jos");
    putfonts8_asc(vram, x, 18, y - 22, red, "jos");
    putfonts8_asc(vram, x, 19, y - 21, purple, "jos");
    // 状态栏
    boxfill8(vram, x, darkgray, x - 47, y - 24, x - 4, y - 24);
    boxfill8(vram, x, darkgray, x - 47, y - 23, x - 47, y - 4);
    boxfill8(vram, x, white, x - 47, y - 3, x - 4, y - 3);
    boxfill8(vram, x, white, x - 3, y - 24, x - 3, y - 3);
    return;
}

void putfont8(unsigned char *const vram, int const xsize, int const x, int const y, char const c, char const *const font)
{
    /*
    根据点阵数据显示单个 8*16 文字
    */
    int i;
    char d;
    unsigned char *p = vram + y * xsize + x;
    for (i = 0; i < 16; i++)
    {
        d = font[i];
        if (d & 0x80)
            p[0] = c;
        if (d & 0x40)
            p[1] = c;
        if (d & 0x20)
            p[2] = c;
        if (d & 0x10)
            p[3] = c;
        if (d & 0x08)
            p[4] = c;
        if (d & 0x04)
            p[5] = c;
        if (d & 0x02)
            p[6] = c;
        if (d & 0x01)
            p[7] = c;
        p += xsize;
    }
}

void putfonts8_asc(unsigned char *const vram, const int xsize, int x, const int y, const char c, unsigned char const *s)
{
    /*
    显示字符串
    */
    for (; *s != 0x00; s++)
    {
        putfont8(vram, xsize, x, y, c, font + *s * 16);
        x += 8;
    }
    return;
}

void init_mouse_cursor8(char *const mouse, const char bc)
{
    /*
    准备鼠标指针 16x16。bc：背景色
    */
    static const char cursor[16][16] =
        {
            "**************..",
            "*OOOOOOOOOOO*...",
            "*OOOOOOOOOO*....",
            "*OOOOOOOOO*.....",
            "*OOOOOOOO*......",
            "*OOOOOOO*.......",
            "*OOOOOOO*.......",
            "*OOOOOOOO*......",
            "*OOOO**OOO*.....",
            "*OOO*..*OOO*....",
            "*OO*....*OOO*...",
            "*O*......*OOO*..",
            "**........*OOO*.",
            "*..........*OOO*",
            "............*OO*",
            ".............***",
        };
    int x, y;
    for (y = 0; y < 16; y++)
        for (x = 0; x < 16; x++)
        {
            if (cursor[y][x] == '*')
                mouse[y * 16 + x] = black;
            else if (cursor[y][x] == 'O')
                mouse[y * 16 + x] = white;
            else
                mouse[y * 16 + x] = bc;
        }
    return;
}

void putblock8_8(unsigned char *const vram, const int vxsize,
                 const int pxsize, const int pysize,
                 const int px0, const int py0,
                 char const *const buf, const int bxsize)
{
    /*
    将缓冲区内数据拷贝到屏幕上。
    vxsize: 屏幕的x尺寸
    pxsize,pysize: 要放置到的屏幕区域尺寸 (px0,py0)左上位置
    buf: 缓冲区 bxsize: 缓冲区的x尺寸
    */
    int x, y;
    for (y = 0; y < pysize; y++)
        for (x = 0; x < pxsize; x++)
            vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x];
    return;
}
