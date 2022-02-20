[INSTRSET "i486p"]
[BITS 32]
    MOV     ECX, msg
    MOV     EDX, 1
putloop:
    MOV     AL, [CS:ECX]
    CMP     AL, 0
    JE      fin
    INT     0x40
    INC     ECX
    JMP     putloop
fin:
    MOV     EDX, 4          ; 退出信号
    INT     0x40
msg:
    DB      "Hello", 0
