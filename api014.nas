; 14. 关闭窗口（EBX=窗口句柄）

[FORMAT "WCOFF"]                ; 生成对象文件的格式
[INSTRSET "i486p"]              ; 486兼容指令集
[BITS 32]                       ; 生成32位模式机器语言
[FILE "api014.nas"]             ; 源文件名

    GLOBAL  _api_closewin

[SECTION .text]

_api_closewin:		; void api_closewin(int win);
    PUSH	EBX
    MOV		EDX, 14
    MOV		EBX, [ESP+8]
    INT		0x40
    POP		EBX
    RET
