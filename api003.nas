; 3. 显示某长度字符串（EBX=字符串地址，ECX=字符串长度）

[FORMAT "WCOFF"]                ; 生成对象文件的格式
[INSTRSET "i486p"]              ; 486兼容指令集
[BITS 32]                       ; 生成32位模式机器语言
[FILE "api003.nas"]             ; 源文件名

    GLOBAL  _api_putstr1

[SECTION .text]

_api_putstr1:       ; void api_putstr1(char *s, int l);
    PUSH    EBX
    MOV     EDX, 3
    MOV     EBX, [ESP+8]        ; PUSH时减了4
    MOV     ECX, [ESP+12]
    INT     0x40
    POP     EBX
    RET
