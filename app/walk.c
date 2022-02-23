#include "apilib.h"

void Main(void)
{
    api_initmalloc();
    char *buf = api_malloc(160 * 100);
    int win = api_openwin(buf, 160, 100, -1, "walk");
    api_boxfilwin(win, 4, 24, 155, 95, 0 /*黑色*/); // 刷新
    int x = 76, y = 56;
    api_putstrwin(win, x, y, 3 /*黄*/, 1, "*");
    int i;
    for (;;)
    {
        i = api_getkey(1);
        api_putstrwin(win, x, y, 0 /*黑*/, 1, "*"); // 用黑色擦除
        if (i == '4' && x > 4)
            x -= 8;
        else if (i == '6' && x < 148)
            x += 8;
        else if (i == '8' && y > 24)
            y -= 8;
        else if (i == '2' && y < 80)
            y += 8;
        else if (i == 0x0a)
            break;
        api_putstrwin(win, x, y, 3 /*黄*/, 1, "*");
    } // 回车键结束
    api_closewin(win);
    api_end();
}
