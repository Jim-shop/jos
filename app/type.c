#include "apilib.h"

void Main(void)
{
    char cmdline[30];
    api_getcmdline(cmdline, 30);
    char *p;
    for (p = cmdline; *p != ' '; p++)
        ; // 跳到第一个空格处
    for (; *p == ' '; p++)
        ; // 跳过空格
    int fh = api_fopen(p);
    char c;
    if (fh != 0)
    {
        for (;;)
        {
            if (api_fread(&c, 1, fh) == 0)
                break;
            api_putchar(c);
        }
    }
    else
        api_putstr0("File not found.\n");
    api_end();
}
