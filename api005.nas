; 5. 显示窗口（EBX=窗口缓冲区，ESI=窗口x大小，EDI=窗口y大小，EAX=透明色，ECX=窗口名称）->EAX=窗口句柄

[FORMAT "WCOFF"]                ; 生成对象文件的格式
[INSTRSET "i486p"]              ; 486兼容指令集
[BITS 32]                       ; 生成32位模式机器语言
[FILE "api005.nas"]             ; 源文件名

    GLOBAL  _api_openwin

[SECTION .text]

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
