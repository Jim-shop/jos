/*
写主函数
*/

#include "bootpack.h"

struct BOOTINFO *const binfo = (struct BOOTINFO *)ADR_BOOTINFO;

void HariMain(void)
{
    // 初始化中断
    init_gdtidt();
    init_pic();
    // 中断初始化完成，开放中断
    io_sti();

    // 初始化界面
    init_palette();
    init_screen8(binfo->VRAM, binfo->SCRNX, binfo->SCRNY);

    char mcursor[256];
    int mx = (binfo->SCRNX - 16) / 2;
    int my = (binfo->SCRNY - 28 - 16) / 2;
    init_mouse_cursor8(mcursor, lightdarkblue);
    putblock8_8(binfo->VRAM, binfo->SCRNX, 16, 16, mx, my, mcursor, 16);

    char s[40];
    sprintf(s, "(%d, %d)", mx, my);
    putfonts8_asc(binfo->VRAM, binfo->SCRNX, 0, 0, white, s);

    io_out8(PIC0_IMR, 0xf9); // 11111001 开放PIC1和键盘中断
    io_out8(PIC1_IMR, 0xef); // 11101111 开放鼠标中断

    for (;;)
        io_hlt();
}
