; 1. 显示单个字符（AL=字符编码）

[FORMAT "WCOFF"]                ; 生成对象文件的格式
[INSTRSET "i486p"]              ; 486兼容指令集
[BITS 32]                       ; 生成32位模式机器语言
[FILE "api001.nas"]             ; 源文件名

    GLOBAL  _api_putchar

[SECTION .text]

_api_putchar:       ; void api_putchar(int c);
    MOV     EDX, 1
    MOV     AL, [ESP+4]
    INT     0x40
    RET
