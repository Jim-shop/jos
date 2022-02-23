; 9. malloc（EBX=memman地址，ECX=请求字节数）->EAX=分配到的内存地址

[FORMAT "WCOFF"]                ; 生成对象文件的格式
[INSTRSET "i486p"]              ; 486兼容指令集
[BITS 32]                       ; 生成32位模式机器语言
[FILE "api009.nas"]             ; 源文件名

    GLOBAL  _api_malloc

[SECTION .text]

_api_malloc:        ; char *api_malloc(int size);
    PUSH    EBX
    MOV     EDX, 9
    MOV     EBX, [CS:0X0020]
    MOV     ECX, [ESP+8]        ; size
    INT     0X40
    POP     EBX
    RET
