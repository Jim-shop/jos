; 10. free（EBX=memman地址，EAX=释放的地址，ECX=释放字节数）

[FORMAT "WCOFF"]                ; 生成对象文件的格式
[INSTRSET "i486p"]              ; 486兼容指令集
[BITS 32]                       ; 生成32位模式机器语言
[FILE "api010.nas"]             ; 源文件名

    GLOBAL  _api_free

[SECTION .text]

_api_free:			; void api_free(char *addr, int size);
    PUSH    EBX
    MOV     EDX, 10
    MOV     EBX, [CS:0X0020]
    MOV     EAX, [ESP+8]        ; addr
    MOV     ECX, [ESP+12]
    INT     0X40
    POP     EBX
    RET
    