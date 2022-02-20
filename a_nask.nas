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
; 6. 窗口中叠写字符串（EBX=窗口句柄，(ESI,EDI)=(x,y)，EAX=色号，ECX=字符串长度，EBP=字符串）
; 7. 窗口中绘制方块（EBX=窗口句柄，(EAX,ECX)=(x0,y0)，(ESI,EDI)=(x1,y1)不超尾，EBP=色号）


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

[SECTION .text]

_api_putchar:   ; void api_putchar(int c);
    MOV     EDX, 1
    MOV     AL, [ESP+4]
    INT     0x40
    RET
    
_api_putstr0:   ; void api_putstr0(char *s);
    PUSH    EBX
    MOV     EDX, 2
    MOV     EBX, [ESP+8]        ; PUSH时减了4
    INT     0x40
    POP     EBX
    RET

_api_end:       ; void api_end(void);
    MOV     EDX, 4
    INT     0x40

_api_openwin:   ; int api_openwin(char *buf, int xsiz, int ysiz, int col_inv, char *title);
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

_api_putstrwin:	; void api_putstrwin(int win, int x, int y, int col, int len, char *str);
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

_api_boxfilwin:	; void api_boxfilwin(int win, int x0, int y0, int x1, int y1, int col);
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
