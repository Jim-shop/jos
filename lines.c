int api_openwin(char *buf, int xsiz, int ysiz, int col_inv, char *title);
void api_initmalloc(void);
char *api_malloc(int size);
void api_refreshwin(int win, int x0, int y0, int x1, int y1);
void api_linewin(int win, int x0, int y0, int x1, int y1, int col);
void api_closewin(int win);
int api_getkey(int mode);
void api_end(void);

void Main(void)
{
    api_initmalloc();
    char *buf = api_malloc(160 * 100);
    int win = api_openwin(buf, 160, 100, -1, "Lines");
    int i;
    for (i = 0; i < 8; i++)
    {
        api_linewin(win + 1, 8, 26, 77, i * 9 + 26, i);  // 不刷新
        api_linewin(win + 1, 88, 26, i * 9 + 88, 89, i); // 不刷新
    }
    api_refreshwin(win, 6, 26, 154, 90);
    for (;;)
        if (api_getkey(1) == '\n')
            break;
    api_closewin(win);
    api_end();
}
