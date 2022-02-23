; 25. 文件读取（EAX=文件句柄，EBX=缓冲区地址，ECX=最大读取字节数）->EAX=读取到的字节数

[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api025.nas"]

    GLOBAL	_api_fread

[SECTION .text]

_api_fread:			; int api_fread(char *buf, int maxsize, int fhandle);
    PUSH	EBX
    MOV		EDX, 25
    MOV		EAX, [ESP+16]		; fhandle
    MOV		ECX, [ESP+12]		; maxsize
    MOV		EBX, [ESP+8]			; buf
    INT		0x40
    POP		EBX
    RET
