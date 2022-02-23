; 13. 窗口中画直线（EBX=窗口句柄(最低bit为0则刷新窗口)，(EAX,ECX)=(x0,y0)，(ESI,EDI)=(x1,y1)不超尾，EBP=色号）

[FORMAT "WCOFF"]                ; 生成对象文件的格式
[INSTRSET "i486p"]              ; 486兼容指令集
[BITS 32]                       ; 生成32位模式机器语言
[FILE "api013.nas"]             ; 源文件名

    GLOBAL  _api_linewin

[SECTION .text]

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
