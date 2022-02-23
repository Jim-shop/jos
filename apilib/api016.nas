; 16. 获取定时器alloc->EAX=定时器句柄

[FORMAT "WCOFF"]                ; 生成对象文件的格式
[INSTRSET "i486p"]              ; 486兼容指令集
[BITS 32]                       ; 生成32位模式机器语言
[FILE "api016.nas"]             ; 源文件名

    GLOBAL	_api_alloctimer

[SECTION .text]

_api_alloctimer:	; int api_alloctimer(void);
    MOV		EDX, 16
    INT		0x40
    RET
