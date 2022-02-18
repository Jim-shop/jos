/*
写主函数
*/

#include "bootpack.h"

// 可显示键位表
const static char keytable0[0x80] = { // 未按下Shift
		0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
		0, 0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']',
		0, 0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`',
		0, '\\', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', 0, '*',
		0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.',
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const static char keytable1[0x80] = { // 按下Shift
		0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
		0, 0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',
		0, 0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
		0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*',
		0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.',
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

struct BOOTINFO *const binfo = (struct BOOTINFO *const)ADR_BOOTINFO;

void make_window8(unsigned char *const buf, const int xsize, const int ysize, char const *const title, char const act);
void make_wtitle8(unsigned char *const buf, const int xsize, char const *const title, char const act);
void make_textbox8(struct SHEET *const sht, const int x0, const int y0, const int sx, const int sy, const int c);
void putfonts8_asc_sht(struct SHEET *const sht, const int x, const int y, const int c, const int b, char const *const s, const int l);
// 各个主函数不要使用return语句，因为return返回[ESP]地址，而[ESP]这里没有写入
void console_task(struct SHEET *const sheet, unsigned int const memtotal);
int cons_newline(int cursor_y, struct SHEET *const sheet);

struct FILEINFO
{																			// FAT12文件表单项内容（最多224个）
	unsigned char name[8];							// 文件名。第一字节0xe5代表删除了,0x00代表不包含文件信息。
	unsigned char ext[3];								// 扩展名
	unsigned char type;									// 属性信息：0x00/0x20一般文件0x01只读0x02隐藏0x04系统0x08非文件信息(磁盘名称等)0x10目录（可以叠加）
	unsigned char reserve[10];					// 保留无用的10字节
	unsigned short time, date, clustno; // 时间，日期，簇号
	unsigned int size;									// 大小
};

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
	task_run(task_a, 1, 2);

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
	int cursor_x = 8;													 // 光标位置
	int cursor_c = white;											 // 光标颜色
	struct TIMER *timer_blink = timer_alloc(); // 窗口A光标闪烁定时器
	timer_init(timer_blink, &fifo, 1);
	timer_settime(timer_blink, 50);
	// 控制台窗口图层
	struct SHEET *sht_cons = sheet_alloc(shtctl);
	unsigned char *buf_cons = (unsigned char *)memman_alloc_4k(memman, 256 * 165);
	sheet_setbuf(sht_cons, buf_cons, 256, 165, -1); // 没有透明色
	make_window8(buf_cons, 256, 165, "console", 0);
	make_textbox8(sht_cons, 8, 28, 240, 128, black);
	struct TASK *task_cons = task_alloc();
	task_cons->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12; // 申请64k栈并指向栈底，-12方便我们放位于ESP+4、ESP+8的两个4字节C语言参数
	*((int *)(task_cons->tss.esp + 4)) = (int)sht_cons;												// 将sht_back地址作为B的第一个参数
	*((int *)(task_cons->tss.esp + 8)) = memtotal;														// B的第二个参数
	task_cons->tss.eip = (int)&console_task;
	task_cons->tss.es = 1 * 8;
	task_cons->tss.cs = 2 * 8;
	task_cons->tss.ss = 1 * 8;
	task_cons->tss.ds = 1 * 8;
	task_cons->tss.fs = 1 * 8;
	task_cons->tss.gs = 1 * 8;
	task_run(task_cons, 2, 2);
	// 设定图层位置
	sheet_slide(sht_back, 0, 0); // 此时图层还不可见，不会绘图
	sheet_slide(sht_mouse, mx, my);
	sheet_slide(sht_win, 64, 56);
	sheet_slide(sht_cons, 40, 64);
	// 设定图层高度
	sheet_updown(sht_back, 0);
	sheet_updown(sht_cons, 1);
	sheet_updown(sht_win, 2);
	sheet_updown(sht_mouse, 3);

	// 开放中断
	io_out8(PIC0_IMR, 0xf8); // 11111000 开放时钟、从PIC、键盘中断
	io_out8(PIC1_IMR, 0xef); // 11101111 开放鼠标中断

	// 发往键盘的数据
	int keycmd_buf[32];
	struct FIFO32 keycmd;
	int keycmd_wait = -1; // 消息发送成功的话置-1
	fifo32_init(&keycmd, 32, keycmd_buf, 0);
	// 决定键盘输入发给哪个窗口
	int key_to = 0;
	// 左Shift按下=1，右Shift按下=2，同时按下=3，不按=0
	int key_shift = 0;
	// binfo->LEDS: 4，5，6位依次是ScrollLock, NumLock, CapsLock
	int key_leds = (binfo->LEDS >> 4) & 7; // 7=0111b
	fifo32_put(&keycmd, KEYCMD_LED);
	fifo32_put(&keycmd, key_leds);
	/*
	键盘LED控制：
	1. 读取状态寄存器等待bit 1变为0(wait_KBC_sendready)
	2. 向数据输出(PORT_KEYDAT)写入要发送的1个字节数据
	3. 等待键盘返回一个字节信息(等中断或wait_KBC_sendready)
	4. 返回信息如果为0xfa说明发送成功。0xfe说明发送失败，需返回第一步
	说明：
	要控制LED状态，要按上面方法执行两次，向键盘发送EDxx数据。
	ED就是KEYCMD_LED(0xed)。xx的bit 0,1,2位
	依次是ScrollLock, NumLock, CapsLock，
	0表示熄灭，1表示点亮。
	*/

	for (;;)
	{
		if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0)
		{
			keycmd_wait = fifo32_get(&keycmd);
			wait_KBC_sendready();
			io_out8(PORT_KEYDAT, keycmd_wait);
		}
		io_cli(); // 先屏蔽中断
		if (fifo32_status(&fifo) == 0)
		{
			// io_stihlt(); // 恢复接受中断，待机。当收到中断后就会恢复执行HLT指令之后的指令
			task_sleep(task_a); // 线程休眠
			io_sti();						// 由于线程休眠函数中没有屏蔽中断，所以sti要在其之后，不然sleep可能出问题
		}
		else
		{
			i = fifo32_get(&fifo);
			io_sti();									// 取得消息种类后就可以恢复中断
			if (256 <= i && i <= 511) // 键盘
			{
				// sprintf(s, "%02X", i - 256);
				// putfonts8_asc_sht(sht_back, 0, 16, white, lightdarkblue, s, 2);
				if (i < 256 + 0x80) // 键位表大小0x80
				{
					if (key_shift == 0)
						s[0] = keytable0[i - 256];
					else
						s[0] = keytable1[i - 256];
				}
				else
					s[0] = 0;

				if ('A' <= s[0] && s[0] <= 'Z')
					if (!((key_leds & 4) ^ (key_shift != 0))) // CapsLock和Shift同时On或同时Off时
						s[0] += 0x20;														// 大写转小写

				if (s[0] != 0) // 可显示的字符
				{
					if (key_to == 0) // 发送给任务A
					{
						if (cursor_x < 144)
						{
							s[1] = 0; // 字串终止符
							putfonts8_asc_sht(sht_win, cursor_x, 28, black, white, s, 1);
							cursor_x += 8;
						}
					}
					else // 发送给命令行窗口
					{
						fifo32_put(&task_cons->fifo, s[0] + 256); // 对方的消息队列是字符+偏移256
					}
				}
				else if (i == 256 + 0x0e) // 退格键
				{
					if (key_to == 0) // 发送给任务A
					{
						if (cursor_x > 8)
						{
							putfonts8_asc_sht(sht_win, cursor_x, 28, black, white, " ", 1);
							cursor_x -= 8;
						}
					}
					else // 发送给命令行窗口
					{
						fifo32_put(&task_cons->fifo, 8 + 256); // 指定退格键8
					}
				}
				else if (i == 256 + 0x0f) // Tab键
				{
					if (key_to == 0)
					{
						key_to = 1;
						make_wtitle8(buf_win, sht_win->bxsize, "task_a", 0);
						make_wtitle8(buf_cons, sht_cons->bxsize, "console", 1);
						cursor_c = -1; // 不显示光标
						boxfill8(sht_win->buf, sht_win->bxsize, white, cursor_x, 28, cursor_x + 7, 43);
						fifo32_put(&task_cons->fifo, 2); // 控制台窗口光标ON
					}
					else
					{
						key_to = 0;
						make_wtitle8(buf_win, sht_win->bxsize, "task_a", 1);
						make_wtitle8(buf_cons, sht_cons->bxsize, "console", 0);
						cursor_c = black;								 // 重新显示光标
						fifo32_put(&task_cons->fifo, 3); // 控制台窗口光标OFF
					}
					sheet_refresh(sht_win, 0, 0, sht_win->bxsize, 21);
					sheet_refresh(sht_cons, 0, 0, sht_cons->bxsize, 21);
				}
				else if (i == 256 + 0x1c) // 回车键
				{
					if (key_to != 0) // 发送到命令行窗口
						fifo32_put(&task_cons->fifo, 10 + 256);
				}
				else if (i == 256 + 0x2a) // 左Shift ON
					key_shift |= 1;
				else if (i == 256 + 0x36) // 右Shift ON
					key_shift |= 2;
				else if (i == 256 + 0xaa) // 左Shift OFF
					key_shift &= ~1;
				else if (i == 256 + 0xb6) // 右Shift OFF
					key_shift &= ~2;
				else if (i == 256 + 0x3a) // CapsLock
				{
					key_leds ^= 4;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				else if (i == 256 + 0x45) // NumLock
				{
					key_leds ^= 2;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				else if (i == 256 + 0x46) // ScrollLock
				{
					key_leds ^= 1;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				else if (i == 256 + 0xfa) // 键盘成功接收数据
					keycmd_wait = -1;
				else if (i == 256 + 0xfe) // 键盘接收数据失败
				{
					wait_KBC_sendready();
					io_out8(PORT_KEYDAT, keycmd_wait);
				}
				if (cursor_c >= 0) // 光标显示
					boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
				sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
			}
			else if (512 <= i && i <= 767) // 鼠标
			{
				if (mouse_decode(&mdec, i - 512) != 0)
				{
					// // 鼠标按键信息绘制
					// sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					// if (mdec.btn & 0x01)
					//   s[1] = 'L';
					// if (mdec.btn & 0x02)
					//   s[3] = 'R';
					// if (mdec.btn & 0x04)
					//   s[2] = 'C';
					// putfonts8_asc_sht(sht_back, 32, 16, white, lightdarkblue, s, 15);
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
					// sprintf(s, "(%3d, %3d)", mx, my);
					// putfonts8_asc_sht(sht_back, 0, 0, white, lightdarkblue, s, 10);
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
					if (cursor_c >= 0)
						cursor_c = black;
				}
				else
				{
					timer_init(timer_blink, &fifo, 1);
					if (cursor_c >= 0)
						cursor_c = white;
				}
				timer_settime(timer_blink, 50);
				if (cursor_c >= 0)
				{
					boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
					sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
				}
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

void make_wtitle8(unsigned char *const buf, const int xsize, char const *const title, char const act)
{
	/*
	在buf中画窗口的标题栏。
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
	if (act)
	{
		tc = white;
		tbc = darkblue;
	}
	else
	{
		tc = gray;
		tbc = darkgray;
	}
	boxfill8(buf, xsize, tbc, 3, 3, xsize - 4, 20);
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

void make_window8(unsigned char *const buf, const int xsize, const int ysize, char const *const title, char const act)
{
	/*
	制造窗口的buf
	*/
	boxfill8(buf, xsize, gray, 0, 0, xsize - 1, 0);
	boxfill8(buf, xsize, white, 1, 1, xsize - 2, 1);
	boxfill8(buf, xsize, gray, 0, 0, 0, ysize - 1);
	boxfill8(buf, xsize, white, 1, 1, 1, ysize - 2);
	boxfill8(buf, xsize, darkgray, xsize - 2, 1, xsize - 2, ysize - 2);
	boxfill8(buf, xsize, black, xsize - 1, 0, xsize - 1, ysize - 1);
	boxfill8(buf, xsize, gray, 2, 2, xsize - 3, ysize - 3);
	boxfill8(buf, xsize, darkgray, 1, ysize - 2, xsize - 2, ysize - 2);
	boxfill8(buf, xsize, black, 0, ysize - 1, xsize - 1, ysize - 1);
	make_wtitle8(buf, xsize, title, act);
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

int cons_newline(int cursor_y, struct SHEET *const sheet)
{
	/*
	给控制台换行。行满时滚动。返回换行之后cursor_y位置。
	使用前记得把光标擦除。
	*/
	int x, y;
	if (cursor_y < 28 + 112) // 没填满所有行
		cursor_y += 16;				 // 换行
	else										 // 填满了所有行
	{
		for (y = 28; y < 28 + 112; y++)
			for (x = 8; x < 8 + 240; x++)
				sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize]; // 滚动
		for (y = 28 + 112; y < 28 + 128; y++)
			for (x = 8; x < 8 + 240; x++)
				sheet->buf[x + y * sheet->bxsize] = black; // 清空
		sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
	}
	return cursor_y;
}

void console_task(struct SHEET *const sheet, unsigned int const memtotal)
{
	/*
	控制台进程。
	*/

	// 自身task
	struct TASK *task = task_now();

	// 事件队列
	int fifobuf[128];
	fifo32_init(&task->fifo, 128, fifobuf, task);

	// 光标
	int cursor_x = 16, cursor_y = 28, cursor_c = -1;
	struct TIMER *timer = timer_alloc();
	timer_init(timer, &task->fifo, 1);
	timer_settime(timer, 50);

	// 暂存指令
	char cmdline[30];

	// 临时变量
	int x, y;
	char s[30];

	// mem 指令用
	struct MEMMAN const *const memman = (struct MEMMAN const *const)MEMMAN_ADDR;

	// dir 指令用
	struct FILEINFO *const finfo = (struct FILEINFO *const)(ADR_DISKIMG + 0x002600);

	// 提示符
	putfonts8_asc_sht(sheet, 8, 28, white, black, ">", 1);

	int i;
	for (;;)
	{
		io_cli();
		if (fifo32_status(&task->fifo) == 0)
		{
			task_sleep(task);
			io_sti();
		}
		else
		{
			i = fifo32_get(&task->fifo);
			io_sti();
			if (i <= 1) // 光标闪烁
			{
				if (i != 0)
				{
					timer_init(timer, &task->fifo, 0);
					if (cursor_c >= 0)
						cursor_c = white;
				}
				else
				{
					timer_init(timer, &task->fifo, 1);
					if (cursor_c >= 0)
						cursor_c = black;
				}
				timer_settime(timer, 50);
			}
			else if (i == 2) // 光标ON
				cursor_c = white;
			else if (i == 3) // 光标OFF
			{
				boxfill8(sheet->buf, sheet->bxsize, black, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
				cursor_c = -1;
			}
			else if (256 <= i && i <= 511) // 键盘数据（从任务A得到）
			{
				if (i == 8 + 256) // 退格键
				{
					if (cursor_x > 16)
					{
						putfonts8_asc_sht(sheet, cursor_x, cursor_y, white, black, " ", 1);
						cursor_x -= 8;
					}
				}
				else if (i == 10 + 256) // 回车键
				{
					putfonts8_asc_sht(sheet, cursor_x, cursor_y, white, black, " ", 1); // 擦除光标
					cmdline[cursor_x / 8 - 2] = 0;																			// 字串结束标志
					cursor_y = cons_newline(cursor_y, sheet);														// 换行
					if (strcmp(cmdline, "mem") == 0)																		// mem指令
					{																																		// 指令功能：查询总计内存和剩余内存
						sprintf(s, "total   %dMB", memtotal / (1024 * 1024));
						putfonts8_asc_sht(sheet, 8, cursor_y, white, black, s, 30);
						cursor_y = cons_newline(cursor_y, sheet);
						sprintf(s, "free %dKB", memman_total(memman) / 1024);
						putfonts8_asc_sht(sheet, 8, cursor_y, white, black, s, 30);
						cursor_y = cons_newline(cursor_y, sheet);
						cursor_y = cons_newline(cursor_y, sheet);
					}
					else if (strcmp(cmdline, "cls") == 0) // cls 命令
					{																			// 功能：清屏
						for (y = 28; y < 28 + 128; y++)
							for (x = 8; x < 8 + 240; x++)
								sheet->buf[x + y * sheet->bxsize] = black;
						sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
						cursor_y = 28;
					}
					else if (strcmp(cmdline, "dir") == 0) // dir 命令
					{																			// 显示文件信息
						for (x = 0; x < 224; x++)						// 文件表中最多224项文件
						{
							if (finfo[x].name[0] == 0x00) // 没有文件信息了
								break;
							if (finfo[x].name[0] != 0xe5) // 不是已删除的文件
							{
								if ((finfo[x].type & 0x18) == 0) // 不是目录或非文件信息
								{
									sprintf(s, "filename.ext   %7d", finfo[x].size);
									for (y = 0; y < 8; y++)
										s[y] = finfo[x].name[y]; // 文件名
									s[9] = finfo[x].ext[0];
									s[10] = finfo[x].ext[1];
									s[11] = finfo[x].ext[2]; // 扩展名
									putfonts8_asc_sht(sheet, 8, cursor_y, white, black, s, 30);
									cursor_y = cons_newline(cursor_y, sheet);
								}
							}
						}
						cursor_y = cons_newline(cursor_y, sheet);
					}
					else if (cmdline[0] != 0)
					{ // 不是命令也不是空行
						putfonts8_asc_sht(sheet, 8, cursor_y, white, black, "Wrong instruction.", 19);
						cursor_y = cons_newline(cursor_y, sheet);
						cursor_y = cons_newline(cursor_y, sheet);
					}
					putfonts8_asc_sht(sheet, 8, cursor_y, white, black, ">", 1); // 显示提示符
					cursor_x = 16;
				}
				else // 可显示字符
				{
					if (cursor_x < 240)
					{
						s[0] = i - 256;
						s[1] = 0;
						cmdline[cursor_x / 8 - 2] = i - 256; // 读入
						putfonts8_asc_sht(sheet, cursor_x, cursor_y, white, black, s, 1);
						cursor_x += 8;
					}
				}
			}
			// 光标
			if (cursor_c >= 0)
				boxfill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
			sheet_refresh(sheet, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16);
		}
	}
}
