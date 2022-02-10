extern void io_hlt(void);
extern void io_cli(void);
extern void io_out8(int port, int data);
extern int io_load_eflags(void);
extern void io_store_eflags(int eflags);

void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *const vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);

void HariMain(void)
{
    unsigned char *const vram = (char *)0xa0000;
    init_palette();
    const int xsize = 320;
    const int ysize = 200;
    boxfill8(vram, xsize, 14,          0, 0,          xsize -  1, ysize - 29);
	boxfill8(vram, xsize,  8,          0, ysize - 28, xsize -  1, ysize - 28);
	boxfill8(vram, xsize,  7,          0, ysize - 27, xsize -  1, ysize - 27);
	boxfill8(vram, xsize,  8,          0, ysize - 26, xsize -  1, ysize -  1);

	boxfill8(vram, xsize,  7,          3, ysize - 24,         59, ysize - 24);
	boxfill8(vram, xsize,  7,          2, ysize - 24,          2, ysize -  4);
	boxfill8(vram, xsize, 15,          3, ysize -  4,         59, ysize -  4);
	boxfill8(vram, xsize, 15,         59, ysize - 23,         59, ysize -  5);
	boxfill8(vram, xsize,  0,          2, ysize -  3,         59, ysize -  3);
	boxfill8(vram, xsize,  0,         60, ysize - 24,         60, ysize -  3);

	boxfill8(vram, xsize, 15, xsize - 47, ysize - 24, xsize -  4, ysize - 24);
	boxfill8(vram, xsize, 15, xsize - 47, ysize - 23, xsize - 47, ysize -  4);
	boxfill8(vram, xsize,  7, xsize - 47, ysize -  3, xsize -  4, ysize -  3);
	boxfill8(vram, xsize,  7, xsize -  3, ysize - 24, xsize -  3, ysize -  3);

    for (;;)
        io_hlt();
}

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

void set_palette(int start, int end, unsigned char *rgb)
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
    int i;
    int eflags = io_load_eflags(); // 记录中断许可标志的值
    io_cli();                      // 将中断许可标志置0，禁止中断
    io_out8(0x03c8, start);
    for (i = start; i <= end; i++)
    {
        io_out8(0x03c9, rgb[0] / 4);
        io_out8(0x03c9, rgb[1] / 4);
        io_out8(0x03c9, rgb[2] / 4);
        rgb += 3;
    }
    io_store_eflags(eflags); // 恢复中断许可标志
    return;
}

void boxfill8(unsigned char *const vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1)
{
    int x, y;
    for (y = y0; y <= y1; y++)
        for (x = x0; x <= x1; x++)
            vram[y * xsize + x] = c;
    return;
}
