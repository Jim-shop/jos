; 12. 刷新窗口（EBX=窗口句柄，(EAX,ECX)=(x0,y0)，(ESI,EDI)=(x1,y1)超尾）

[FORMAT "WCOFF"]                ; 生成对象文件的格式
[INSTRSET "i486p"]              ; 486兼容指令集
[BITS 32]                       ; 生成32位模式机器语言
[FILE "api012.nas"]             ; 源文件名

    GLOBAL  _api_refreshwin

[SECTION .text]

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
