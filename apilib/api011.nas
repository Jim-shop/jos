; 11. 窗口中画点（EBX=窗口句柄(最低bit为0则刷新窗口)，(ESI,EDI)=(x,y)，EAX=色号）

[FORMAT "WCOFF"]                ; 生成对象文件的格式
[INSTRSET "i486p"]              ; 486兼容指令集
[BITS 32]                       ; 生成32位模式机器语言
[FILE "api011.nas"]             ; 源文件名

    GLOBAL  _api_point

[SECTION .text]

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
