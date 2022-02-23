; 26. 获取命令行（EBX=储存地址，ECX=获取的字节数）->EAX=实际存放的字节数

[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api026.nas"]

    GLOBAL	_api_getcmdline

[SECTION .text]

_api_getcmdline:		; int api_getcmdline(char *buf, int maxsize);
    PUSH	EBX
    MOV		EDX, 26
    MOV		ECX, [ESP+12]	
    MOV		EBX, [ESP+8]	
    INT		0x40
    POP		EBX
    RET
