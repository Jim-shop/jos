; 17. 设置定时器的发送数据init（EBX=定时器句柄，EAX=数据）

[FORMAT "WCOFF"]                ; 生成对象文件的格式
[INSTRSET "i486p"]              ; 486兼容指令集
[BITS 32]                       ; 生成32位模式机器语言
[FILE "api017.nas"]             ; 源文件名

    GLOBAL	_api_inittimer

[SECTION .text]

_api_inittimer:		; void api_inittimer(int timer, int data);
    PUSH	EBX
    MOV		EDX, 17
    MOV		EBX, [ESP+8]		; timer
    MOV		EAX, [ESP+12]		; data
    INT		0x40
    POP		EBX
    RET
