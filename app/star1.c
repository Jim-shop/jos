#include "apilib.h"

void Main(void)
{
    api_initmalloc();
    char *buf = api_malloc(150 * 100);
    int win = api_openwin(buf, 150, 100, -1, "star1");
    api_boxfilwin(win, 6, 26, 143, 93, 0 /*黑*/);
    api_point(win, 75, 59, 3 /*黄*/);
    api_end();
}
