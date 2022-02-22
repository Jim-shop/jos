; 对接实现api。
;     api调用方法：
; 1. EDX 填入功能号
; 2. INT 0x40
;     功能号表：
; 1. 显示单个字符（AL=字符编码）
; 2. 显示以'\0'结尾字符串（EBX=字符串地址）
; 3. 显示某长度字符串（EBX=字符串地址，ECX=字符串长度）
; 4. 结束应用程序
; 5. 显示窗口（EBX=窗口缓冲区，ESI=窗口x大小，EDI=窗口y大小，EAX=透明色，ECX=窗口名称）->EAX=窗口句柄
; 6. 窗口中叠写字符串（EBX=窗口句柄(最低bit为0则刷新窗口)，(ESI,EDI)=(x,y)，EAX=色号，ECX=字符串长度，EBP=字符串）
; 7. 窗口中绘制方块（EBX=窗口句柄(最低bit为0则刷新窗口)，(EAX,ECX)=(x0,y0)，(ESI,EDI)=(x1,y1)不超尾，EBP=色号）
; 8. memman初始化（EBX=memman地址，EAX=空间地址，ECX=空间大小）
; 9. malloc（EBX=memman地址，ECX=请求字节数）->EAX=分配到的内存地址
; 10. free（EBX=memman地址，EAX=释放的地址，ECX=释放字节数）
; 11. 窗口中画点（EBX=窗口句柄(最低bit为0则刷新窗口)，(ESI,EDI)=(x,y)，EAX=色号）
; 12. 刷新窗口（EBX=窗口句柄，(EAX,ECX)=(x0,y0)，(ESI,EDI)=(x1,y1)超尾）
; 13. 窗口中画直线（EBX=窗口句柄(最低bit为0则刷新窗口)，(EAX,ECX)=(x0,y0)，(ESI,EDI)=(x1,y1)不超尾，EBP=色号）
; 14. 关闭窗口（EBX=窗口句柄）
; 15. 获取键盘/定时器输入（EAX={0:不阻塞，没有输入时返回-1; 1:阻塞}）->EAX=输入的字符编码
; 16. 获取定时器alloc->EAX=定时器句柄
; 17. 设置定时器的发送数据init（EBX=定时器句柄，EAX=数据）
; 18. 定时器时间设定set（EBX=定时器句柄，EAX=时间）
; 19. 释放定时器free（EBX=定时器句柄）
; 20. 蜂鸣器发声（EAX=声音频率mHz,0表示停止发声）


[FORMAT "WCOFF"]                ; 生成对象文件的格式
[INSTRSET "i486p"]              ; 486兼容指令集
[BITS 32]                       ; 生成32位模式机器语言
[FILE "a_nask.nas"]             ; 源文件名

    GLOBAL  _api_putchar
    GLOBAL  _api_putstr0
    GLOBAL  _api_end
    GLOBAL  _api_openwin
    GLOBAL  _api_putstrwin
    GLOBAL  _api_boxfilwin
    GLOBAL  _api_initmalloc
    GLOBAL  _api_malloc
    GLOBAL  _api_free
    GLOBAL  _api_point
    GLOBAL  _api_refreshwin
    GLOBAL  _api_linewin
    GLOBAL  _api_closewin
    GLOBAL  _api_getkey
    GLOBAL	_api_alloctimer
    GLOBAL	_api_inittimer
    GLOBAL	_api_settimer
    GLOBAL	_api_freetimer
    GLOBAL	_api_beep

[SECTION .text]

_api_putchar:       ; void api_putchar(int c);
    MOV     EDX, 1
    MOV     AL, [ESP+4]
    INT     0x40
    RET
    
_api_putstr0:       ; void api_putstr0(char *s);
    PUSH    EBX
    MOV     EDX, 2
    MOV     EBX, [ESP+8]        ; PUSH时减了4
    INT     0x40
    POP     EBX
    RET

_api_end:           ; void api_end(void);
    MOV     EDX, 4
    INT     0x40

_api_openwin:       ; int api_openwin(char *buf, int xsiz, int ysiz, int col_inv, char *title);
    PUSH    EDI
    PUSH    ESI
    PUSH    EBX
    MOV     EDX, 5
    MOV     EBX, [ESP+16]       ; 前面PUSH了三个DWORD
    MOV     ESI, [ESP+20]
    MOV     EDI, [ESP+24]
    MOV     EAX, [ESP+28]
    MOV     ECX, [ESP+32]
    INT     0X40
    POP     EBX
    POP     ESI
    POP     EDI
    RET

_api_putstrwin:	    ; void api_putstrwin(int win, int x, int y, int col, int len, char *str);
    PUSH	EDI
    PUSH	ESI
    PUSH	EBP
    PUSH	EBX
    MOV		EDX, 6
    MOV		EBX, [ESP+20]
    MOV		ESI, [ESP+24]
    MOV		EDI, [ESP+28]
    MOV		EAX, [ESP+32]
    MOV		ECX, [ESP+36]
    MOV		EBP, [ESP+40]
    INT		0x40
    POP		EBX
    POP		EBP
    POP		ESI
    POP		EDI
    RET

_api_boxfilwin:	    ; void api_boxfilwin(int win, int x0, int y0, int x1, int y1, int col);
    PUSH	EDI
    PUSH	ESI
    PUSH	EBP
    PUSH	EBX
    MOV		EDX, 7
    MOV		EBX, [ESP+20]
    MOV		EAX, [ESP+24]
    MOV		ECX, [ESP+28]
    MOV		ESI, [ESP+32]
    MOV		EDI, [ESP+36]
    MOV		EBP, [ESP+40]
    INT		0x40
    POP		EBX
    POP		EBP
    POP		ESI
    POP		EDI
    RET

_api_initmalloc:	; void api_initmalloc(void);
    PUSH    EBX
    MOV     EDX, 8
    MOV     EBX, [CS:0x0020]    ; 程序格式文件中malloc内存空间的地址
    MOV     EAX, EBX
    ADD     EAX, 32*1024        ; +32KB用来存放malloc表
    MOV     ECX, [CS:0X0000]    ; 程序文件格式中数据段的大小
    SUB     ECX, EAX            ; 减掉malloc表占用的大小
    INT     0X40
    POP     EBX
    RET

_api_malloc:        ; char *api_malloc(int size);
    PUSH    EBX
    MOV     EDX, 9
    MOV     EBX, [CS:0X0020]
    MOV     ECX, [ESP+8]        ; size
    INT     0X40
    POP     EBX
    RET

_api_free:			; void api_free(char *addr, int size);
    PUSH    EBX
    MOV     EDX, 10
    MOV     EBX, [CS:0X0020]
    MOV     EAX, [ESP+8]        ; addr
    MOV     ECX, [ESP+12]
    INT     0X40
    POP     EBX
    RET

_api_point:		    ; void api_point(int win, int x, int y, int col);
    PUSH    EDI
    PUSH    ESI
    PUSH    EBX
    MOV     EDX, 11
    MOV     EBX, [ESP+16]
    MOV     ESI, [ESP+20]
    MOV     EDI, [ESP+24]
    MOV     EAX, [ESP+28]
    INT     0X40
    POP     EBX
    POP     ESI
    POP     EDI
    RET

_api_refreshwin:	; void api_refreshwin(int win, int x0, int y0, int x1, int y1);
    PUSH	EDI
    PUSH	ESI
    PUSH	EBX
    MOV		EDX, 12
    MOV		EBX, [ESP+16]
    MOV		EAX, [ESP+20]
    MOV		ECX, [ESP+24]
    MOV		ESI, [ESP+28]
    MOV		EDI, [ESP+32]
    INT		0x40
    POP		EBX
    POP		ESI
    POP		EDI
    RET

_api_linewin:		; void api_linewin(int win, int x0, int y0, int x1, int y1, int col);
    PUSH	EDI
    PUSH	ESI
    PUSH	EBP
    PUSH	EBX
    MOV		EDX, 13
    MOV		EBX, [ESP+20]
    MOV		EAX, [ESP+24]
    MOV		ECX, [ESP+28]
    MOV		ESI, [ESP+32]
    MOV		EDI, [ESP+36]
    MOV		EBP, [ESP+40]
    INT		0x40
    POP		EBX
    POP		EBP
    POP		ESI
    POP		EDI
    RET

_api_closewin:		; void api_closewin(int win);
    PUSH	EBX
    MOV		EDX, 14
    MOV		EBX, [ESP+8]
    INT		0x40
    POP		EBX
    RET

_api_getkey:		; int api_getkey(int mode);
    MOV		EDX, 15
    MOV		EAX, [ESP+4]
    INT		0x40
    RET

_api_alloctimer:	; int api_alloctimer(void);
    MOV		EDX, 16
    INT		0x40
    RET

_api_inittimer:		; void api_inittimer(int timer, int data);
    PUSH	EBX
    MOV		EDX, 17
    MOV		EBX, [ESP+8]		; timer
    MOV		EAX, [ESP+12]		; data
    INT		0x40
    POP		EBX
    RET

_api_settimer:		; void api_settimer(int timer, int time);
    PUSH	EBX
    MOV		EDX, 18
    MOV		EBX, [ESP+8]		; timer
    MOV		EAX, [ESP+12]		; time
    INT		0x40
    POP		EBX
    RET

_api_freetimer:		; void api_freetimer(int timer);
    PUSH	EBX
    MOV		EDX, 19
    MOV		EBX, [ESP+8]		; timer
    INT		0x40
    POP		EBX
    RET

_api_beep:			; void api_beep(int tone);
    MOV		EDX, 20
    MOV		EAX, [ESP+4]		
    INT		0x40
    RET

