; 8. memman初始化（EBX=memman地址，EAX=空间地址，ECX=空间大小）

[FORMAT "WCOFF"]                ; 生成对象文件的格式
[INSTRSET "i486p"]              ; 486兼容指令集
[BITS 32]                       ; 生成32位模式机器语言
[FILE "api008.nas"]             ; 源文件名

    GLOBAL  _api_initmalloc

[SECTION .text]

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
