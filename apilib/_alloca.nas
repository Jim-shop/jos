; 提供C语言所使用的_alloca函数
; _alloca: near-CALL
; 要执行的操作：从栈中分配EAX个字节的内存空间（ESP-=EAX）
; 要遵守的规则：不能改变ECX, EDX, EBX, ESI, EDI的值

[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "_alloca.nas"]

    GLOBAL  __alloca

[SECTION .text]

__alloca:
    ADD     EAX, -4
    SUB     ESP, EAX
    JMP     DWORD [ESP+EAX]     ; ESP被加了，没法直接RET
