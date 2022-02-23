; 23. 文件定位（EAX=文件句柄，ECX=定位起点{0:文件开头; 1:当前位置; 2: 文件末尾}，EBX=偏移量）

[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api023.nas"]

    GLOBAL	_api_fseek

[SECTION .text]

_api_fseek:			; void api_fseek(int fhandle, int offset, int mode);
    PUSH	EBX
    MOV		EDX, 23
    MOV		EAX, [ESP+8]		; fhandle
    MOV		ECX, [ESP+16]		; mode
    MOV		EBX, [ESP+12]		; offset
    INT		0x40
    POP		EBX
    RET
