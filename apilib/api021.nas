; 21. 打开文件（EBX=文件名）->EAX=文件句柄，为0打开失败

[FORMAT "WCOFF"]                ; 生成对象文件的格式
[INSTRSET "i486p"]              ; 486兼容指令集
[BITS 32]                       ; 生成32位模式机器语言
[FILE "api021.nas"]             ; 源文件名

    GLOBAL	_api_fopen

[SECTION .text]

_api_fopen:	        ; int api_fopen(char *fname);
    PUSH    EBX
    MOV		EDX, 21
    MOV		EBX, [ESP+8]		
    INT		0x40
    POP     EBX
    RET
