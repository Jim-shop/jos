[INSTRSET "i486p"]
[BITS 32]
    MOV     EAX, 1*8        ; OS段号
    MOV     DS, AX          
    MOV     BYTE [0X102600], 0
    MOV     EDX, 4
    INT     0x40
    