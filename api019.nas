; 19. 释放定时器free（EBX=定时器句柄）

[FORMAT "WCOFF"]                ; 生成对象文件的格式
[INSTRSET "i486p"]              ; 486兼容指令集
[BITS 32]                       ; 生成32位模式机器语言
[FILE "api019.nas"]             ; 源文件名

    GLOBAL	_api_freetimer

[SECTION .text]

_api_freetimer:		; void api_freetimer(int timer);
    PUSH	EBX
    MOV		EDX, 19
    MOV		EBX, [ESP+8]		; timer
    INT		0x40
    POP		EBX
    RET
