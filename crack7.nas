[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "crack7.nas"]

    GLOBAL _Main

[SECTION .text]

_Main:
    MOV     AX, 0*8+4
    MOV     DS, AX
    CMP     DWORD [DS:0x0004], 'Hari'

    JNE     fin         ; 不是应用程序，不执行任何操作

    MOV     ECX, [DS:0x0000]
    MOV     AX, 1*8+4
    MOV     DS, AX

crackloop:
    ADD     ECX, -1
    MOV     BYTE [DS:ECX], 123
    CMP     ECX, 0
    JNE     crackloop

fin:
    MOV     EDX, 4
    INT     0x40