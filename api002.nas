; 2. 显示以'\0'结尾字符串（EBX=字符串地址）

[FORMAT "WCOFF"]                ; 生成对象文件的格式
[INSTRSET "i486p"]              ; 486兼容指令集
[BITS 32]                       ; 生成32位模式机器语言
[FILE "api002.nas"]             ; 源文件名

    GLOBAL  _api_putstr0

[SECTION .text]

_api_putstr0:       ; void api_putstr0(char *s);
    PUSH    EBX
    MOV     EDX, 2
    MOV     EBX, [ESP+8]        ; PUSH时减了4
    INT     0x40
    POP     EBX
    RET
