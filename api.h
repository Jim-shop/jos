// 1. 显示单个字符
void api_putchar(int c); 
// 2. 显示以'\0'结尾字符串
void api_putstr0(char *s);
// 4. 结束应用程序
void api_end(void);
// 5. 显示窗口（窗口缓冲区，窗口x大小，窗口y大小，透明色，窗口名称）->窗口句柄
int api_openwin(char *buf, int xsiz, int ysiz, int col_inv, char *title);
// 6. 窗口中叠写字符串（窗口句柄(最低bit为0则刷新窗口)，(x,y)，色号，字符串长度，字符串）
void api_putstrwin(int win, int x, int y, int col, int len, char *str);
// 7. 窗口中绘制方块（窗口句柄(最低bit为0则刷新窗口)，(x0,y0)，(x1,y1)不超尾，色号）
void api_boxfilwin(int win, int x0, int y0, int x1, int y1, int col);
// 8. memman初始化（根据程序头信息自动初始化，对齐到16字节）
void api_initmalloc(void);
// 9. malloc（请求字节数->对齐到16字节）->分配到的内存地址
char *api_malloc(int size);
// 10. free（释放的地址，释放字节数->对齐到16字节）
void api_free(char *addr, int size);
// 11. 窗口中画点（窗口句柄(最低bit为0则刷新窗口)，(x,y)，色号）
void api_point(int win, int x, int y, int col);
// 12. 刷新窗口（窗口句柄，(x0,y0)，(x1,y1)超尾）
void api_refreshwin(int win, int x0, int y0, int x1, int y1);
// 13. 窗口中画直线->精度1/1024（窗口句柄(最低bit为0则刷新窗口)，(x0,y0)，(x1,y1)不超尾，色号）
void api_linewin(int win, int x0, int y0, int x1, int y1, int col);
// 14. 关闭窗口（窗口句柄）// 可以自动关闭
void api_closewin(int win);
// 15. 获取键盘/定时器输入（模式{0:不阻塞，没有输入时返回-1; 1:阻塞}）->输入的字符编码
int api_getkey(int mode);
// 16. 获取定时器alloc->定时器句柄
int api_alloctimer(void);
// 17. 设置定时器的发送数据init（定时器句柄，数据）
void api_inittimer(int timer, int data);
// 18. 定时器时间设定set（定时器句柄，时间）
void api_settimer(int timer, int time);
// 19. 释放定时器free（定时器句柄）
void api_freetimer(int timer);
// 20. 蜂鸣器发声（声音频率mHz,0表示停止发声）
void api_beep(int tone);
