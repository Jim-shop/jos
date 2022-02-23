; 4. 结束应用程序

[FORMAT "WCOFF"]                ; 生成对象文件的格式
[INSTRSET "i486p"]              ; 486兼容指令集
[BITS 32]                       ; 生成32位模式机器语言
[FILE "api004.nas"]             ; 源文件名

    GLOBAL  _api_end

[SECTION .text]

_api_end:           ; void api_end(void);
    MOV     EDX, 4
    INT     0x40
