[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "hello5.nas"]

    GLOBAL _Main

[SECTION .text]

_Main:
    MOV     EDX, 2
    MOV     EBX, msg
    INT     0x40
    MOV     EDX, 4
    INT     0x40

[SECTION .data]

msg:

    DB  "Hello, world", 0x0a, 0
