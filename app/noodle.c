#include <stdio.h>
#include "apilib.h"

void Main(void)
{
    api_initmalloc();
    char *buf = api_malloc(150 * 50);
    int win = api_openwin(buf, 150, 50, -1, "Noodle");
    int timer = api_alloctimer();
    api_inittimer(timer, 128);
    char s[12];
    int hou = 0, min = 0, sec = 0;
    for (;;)
    {
        sprintf(s, "%5d:%02d:%02d", hou, min, sec);
        api_boxfilwin(win, 28, 27, 115, 41, 7);
        api_putstrwin(win, 28, 27, 0, 11, s);
        api_settimer(timer, 100); // 1s
        if (api_getkey(1) != 128) // 刚设的定时器，阻塞模式
            break;
        sec++;
        if (sec == 60)
        {
            sec = 0;
            min++;
            if (min == 60)
            {
                min = 0;
                hou++;
            }
        }
    }
    api_end();
}
