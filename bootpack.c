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
struct MEMMAN *const memman = (struct MEMMAN *const)MEMMAN_ADDR;

int keywin_off(struct SHEET *const key_win, struct SHEET *const sht_win, int cur_c, const int cur_x)
{
	/*
	控制窗口标题栏的颜色为未激活，关闭光标。返回光标颜色。
	*/
	change_wtitle8(key_win, 0); // 标题栏切换到不激活颜色
	if (key_win == sht_win)
	{
		cur_c = -1; // 删除光标
		boxfill8(sht_win->buf, sht_win->bxsize, white, cur_x, 28, cur_x + 7, 43);
	}
	else
	{
		if (key_win->flags & 0x20)				 // 有光标
			fifo32_put(&key_win->task->fifo, 3); // 关闭光标
	}
	return cur_c;
}

int keywin_on(struct SHEET *const key_win, struct SHEET const *const sht_win, int cur_c)
{
	/*
	控制窗口标题栏的颜色为激活，打开光标。返回光标颜色。
	*/
	change_wtitle8(key_win, 1); // 标题栏切换到激活颜色
	if (key_win == sht_win)
	{
		cur_c = black; // 显示光标
	}
	else
	{
		if (key_win->flags & 0x20)				 // 有光标
			fifo32_put(&key_win->task->fifo, 2); // 打开光标
	}
	return cur_c;
}

void Main(void)
{
	// 各种用途的临时变量
	int i, j, x, y;
	char s[40];
	struct SHEET *sht = NULL;

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

	// 开放中断
	io_out8(PIC0_IMR, 0xf8); // 11111000 开放时钟、从PIC、键盘中断
	io_out8(PIC1_IMR, 0xef); // 11101111 开放鼠标中断

	// 内存管理初始化
	unsigned int memtotal = memtest(0x00400000, 0xbfffffff); //总内存
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
	*((int *)0x0fe4) = (int)shtctl; // 存一份给api用

	// 鼠标光标图层
	struct SHEET *sht_mouse = sheet_alloc(shtctl);
	unsigned char buf_mouse[256];
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	init_mouse_cursor8(buf_mouse, 99);
	int mx = (binfo->SCRNX - 16) / 2; // 光标位置，取画面中心坐标
	int my = (binfo->SCRNY - 28 - 16) / 2;
	int mmx = -1, mmy = -1; // 鼠标拖动时用来保存原位置

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
	int cursor_x = 8;						   // 光标位置
	int cursor_c = white;					   // 光标颜色
	struct TIMER *timer_blink = timer_alloc(); // 窗口A光标闪烁定时器
	timer_init(timer_blink, &fifo, 1);
	timer_settime(timer_blink, 50);

	// 控制台窗口图层
	struct SHEET *sht_cons = sheet_alloc(shtctl);
	unsigned char *buf_cons = (unsigned char *)memman_alloc_4k(memman, 256 * 165);
	sheet_setbuf(sht_cons, buf_cons, 256, 165, -1); // 没有透明色
	sht_cons->flags |= 0x20;						// 有光标
	make_window8(buf_cons, 256, 165, "console", 0);
	make_textbox8(sht_cons, 8, 28, 240, 128, black);
	// 控制台任务
	struct TASK *task_cons = task_alloc();
	sht_cons->task = task_cons;
	task_cons->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12; // 申请64k栈并指向栈底，-12方便我们放位于ESP+4、ESP+8的两个4字节C语言参数
	*((int *)(task_cons->tss.esp + 4)) = (int)sht_cons;						  // 将sht_back地址作为B的第一个参数
	*((int *)(task_cons->tss.esp + 8)) = memtotal;							  // B的第二个参数
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

	// 决定键盘输入发给哪个窗口
	struct SHEET *key_win = sht_win;

	// 发往键盘的数据
	int keycmd_buf[32];
	struct FIFO32 keycmd;
	int keycmd_wait = -1; // 消息发送成功的话置-1
	fifo32_init(&keycmd, 32, keycmd_buf, 0);
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

	// 控制台光标
	struct CONSOLE *cons;

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
			io_sti();			// 由于线程休眠函数中没有屏蔽中断，所以sti要在其之后，不然sleep可能出问题
		}
		else
		{
			i = fifo32_get(&fifo);
			io_sti();									   // 取得消息种类后就可以恢复中断
			if (key_win->flags == 0)					   // 输入窗口被关闭
			{											   // 决定被激活的窗口
				key_win = shtctl->sheets[shtctl->top - 1]; // 鼠标图层下的最高图层
				cursor_c = keywin_on(key_win, sht_win, cursor_c);
			}
			switch (i)
			{
			case 256 ... 511:		// 键盘
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
					if (((key_leds & 0x4) && key_shift) || // CapsLock和Shift同时On
						(!(key_leds & 0x4) && !key_shift)) // 或同时Off时
						s[0] += 0x20;					   // 大写转小写

				if (s[0] != 0) // 可显示的字符
				{
					if (key_win == sht_win) // 发送给任务A
					{
						if (cursor_x < 144)
						{
							putfont8_sht(sht_win, cursor_x, 28, black, white, s[0]);
							cursor_x += 8;
						}
					}
					else											  // 发送给其他任务
						fifo32_put(&key_win->task->fifo, s[0] + 256); // 对方的消息队列是字符+偏移256
				}
				else // 控制字符按键
				{
					switch (i)
					{
					case 256 + 0x0e:			// 退格键
						if (key_win == sht_win) // 发送给任务A
						{
							if (cursor_x > 8)
							{
								putfont8_sht(sht_win, cursor_x, 28, black, white, ' ');
								cursor_x -= 8;
							}
						}
						else											  // 发送给其他任务
							fifo32_put(&key_win->task->fifo, '\b' + 256); // 发送退格信号
						break;
					case 256 + 0x0f: // Tab键
						cursor_c = keywin_off(key_win, sht_win, cursor_c, cursor_x);
						j = key_win->height - 1; // 切换到下一层的窗口
						if (j == 0)				 // 如果已经是最下（背景层）
							j = shtctl->top - 1; // 就切换到最上（除开鼠标层）
						key_win = shtctl->sheets[j];
						cursor_c = keywin_on(key_win, sht_win, cursor_c);
						break;
					case 256 + 0x1c:			// 回车键
						if (key_win != sht_win) // 发送到其他窗口
							fifo32_put(&key_win->task->fifo, '\n' + 256);
						break;
					case 256 + 0x2a: // 左Shift ON
						key_shift |= 1;
						break;
					case 256 + 0x36: // 右Shift ON
						key_shift |= 2;
						break;
					case 256 + 0xaa: // 左Shift OFF
						key_shift &= ~1;
						break;
					case 256 + 0xb6: // 右Shift OFF
						key_shift &= ~2;
						break;
					case 256 + 0x3a: // CapsLock
						key_leds ^= 4;
						fifo32_put(&keycmd, KEYCMD_LED);
						fifo32_put(&keycmd, key_leds);
						break;
					case 256 + 0x45: // NumLock
						key_leds ^= 2;
						fifo32_put(&keycmd, KEYCMD_LED);
						fifo32_put(&keycmd, key_leds);
						break;
					case 256 + 0x46: // ScrollLock
						key_leds ^= 1;
						fifo32_put(&keycmd, KEYCMD_LED);
						fifo32_put(&keycmd, key_leds);
						break;
					case 256 + 0x3b:							  // F1
						if (key_shift && task_cons->tss.ss0 != 0) // F1 + Shift 且 正在运行应用程序
						{
							cons = (struct CONSOLE *)*((int *)0x0fec);
							cons_putstr0(cons, "\nKeyboard Interrupt.\n");
							io_cli(); // 改变寄存器值时不能被中断
							task_cons->tss.eax = (int)&(task_cons->tss.esp0);
							task_cons->tss.eip = (int)asm_end_app; // 强制终止任务
							io_sti();
						}
						break;
					case 256 + 0x57:										  // F11
						if (shtctl->top > 2)								  // 如果图层数超过2（背景、鼠标各算一个图层）
							sheet_updown(shtctl->sheets[1], shtctl->top - 1); // 把倒数第二个图层翻到第二层（倒数第一是背景，第一是鼠标）
						break;
					case 256 + 0xfa: // 键盘成功接收数据
						keycmd_wait = -1;
						break;
					case 256 + 0xfe: // 键盘接收数据失败
						wait_KBC_sendready();
						io_out8(PORT_KEYDAT, keycmd_wait);
						break;
					}
				}
				if (cursor_c >= 0) // 光标显示
					boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
				sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
				break;

			case 512 ... 767:						   // 鼠标
				if (mouse_decode(&mdec, i - 512) != 0) // 读取成功或意外
				{
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
					sheet_slide(sht_mouse, mx, my);
					if (mdec.btn & 0x01) // 按下左键
					{
						if (mmx < 0) // 如果未处于拖动模式
						{
							for (j = shtctl->top - 1; j > 0; j--) // 自上而下遍历在鼠标图层之下、背景图层之上的图层
							{									  // （sheet_refreshmap因为被鼠标指针挡住了没法用，只能重新遍历）
								sht = shtctl->sheets[j];
								x = mx - sht->vx0;
								y = my - sht->vy0;
								if (0 <= x && x < sht->bxsize && 0 <= y && y < sht->bysize)
								{
									if (sht->buf[y * sht->bxsize + x] != sht->col_inv) // 不是透明
									{
										sheet_updown(sht, shtctl->top - 1);
										if (sht != key_win) // 点选到非激活窗口
										{
											cursor_c = keywin_off(key_win, sht_win, cursor_c, cursor_x);
											key_win = sht;
											cursor_c = keywin_on(key_win, sht_win, cursor_c);
										}
										if (sht->bxsize - 21 <= x && x < sht->bxsize - 5 && 5 <= y && y < 19) // 点到关闭按钮
										{
											if ((sht->flags & 0x10) != 0) // 为应用程序窗口
											{
												cons = (struct CONSOLE *)*((int *)0x0fec);
												cons_putstr0(cons, "\nMouse Interrupt.\n");
												io_cli(); // 强制结束程序
												task_cons->tss.eax = (int)(&task_cons->tss.esp0);
												task_cons->tss.eip = (int)asm_end_app;
												io_sti();
											}
										}
										if (3 <= x && x < sht->bxsize - 3 && 3 <= y && y < 21) // 点到标题栏
										{
											mmx = mx;
											mmy = my; // 进入拖动模式
										}
										break;
									}
								}
							}
						}
						else // 拖动模式
						{	 // 移动窗口
							x = mx - mmx;
							y = my - mmy;
							sheet_slide(sht, sht->vx0 + x, sht->vy0 + y);
							mmx = mx;
							mmy = my;
						}
					}
					else // 没有按下左键
					{
						mmx = -1; // 退出拖动模式
					}
				}
				break;

			case 0 ... 1: // 键盘光标定时器
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
				break;
			}
		}
	}
}
