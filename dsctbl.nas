[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[OPTIMIZE 1]
[OPTION 1]
[BITS 32]
	EXTERN	_load_gdtr
	EXTERN	_asm_inthandler20
	EXTERN	_asm_inthandler21
	EXTERN	_asm_inthandler2c
	EXTERN	_asm_inthandler27
	EXTERN	_load_idtr
[FILE "dsctbl.c"]
	GLOBAL	_gdt
[SECTION .data]
	ALIGNB	4
_gdt:
	DD	2555904
	GLOBAL	_idt
	ALIGNB	4
_idt:
	DD	2553856
[SECTION .text]
	GLOBAL	_init_gdtidt
_init_gdtidt:
	PUSH	EBP
	XOR	EDX,EDX
	MOV	EBP,ESP
	PUSH	ECX
	PUSH	ECX
L8:
	MOV	EAX,DWORD [_gdt]
	LEA	ECX,DWORD [EAX+EDX*8]
	INC	EDX
	CMP	EDX,8191
	MOV	WORD [ECX],0
	MOV	WORD [2+ECX],0
	MOV	BYTE [4+ECX],0
	MOV	BYTE [5+ECX],0
	MOV	BYTE [6+ECX],0
	MOV	BYTE [7+ECX],0
	JLE	L8
	MOV	EAX,DWORD [_gdt]
	LEA	ECX,DWORD [8+EAX]
	MOV	WORD [8+EAX],-1
	MOV	BYTE [4+ECX],0
	MOV	BYTE [5+ECX],-110
	MOV	BYTE [6+ECX],-49
	MOV	BYTE [7+ECX],0
	MOV	EAX,DWORD [_gdt]
	MOV	WORD [2+ECX],0
	LEA	ECX,DWORD [16+EAX]
	MOV	WORD [16+EAX],-1
	MOV	WORD [2+ECX],0
	MOV	BYTE [4+ECX],40
	MOV	BYTE [5+ECX],-102
	MOV	BYTE [6+ECX],71
	MOV	BYTE [7+ECX],0
	PUSH	EDX
	PUSH	EDX
	PUSH	2555904
	PUSH	65535
	CALL	_load_gdtr
	ADD	ESP,16
	XOR	EDX,EDX
L18:
	MOV	EAX,DWORD [_idt]
	LEA	ECX,DWORD [EAX+EDX*8]
	INC	EDX
	CMP	EDX,255
	MOV	WORD [ECX],0
	MOV	WORD [2+ECX],0
	MOV	BYTE [4+ECX],0
	MOV	BYTE [5+ECX],0
	MOV	WORD [6+ECX],0
	JLE	L18
	MOV	ECX,DWORD [_idt]
	MOV	EAX,_asm_inthandler20
	MOV	WORD [256+ECX],AX
	LEA	EDX,DWORD [256+ECX]
	SAR	EAX,16
	MOV	BYTE [4+EDX],0
	MOV	BYTE [5+EDX],-114
	MOV	WORD [6+EDX],AX
	MOV	ECX,DWORD [_idt]
	MOV	WORD [2+EDX],16
	LEA	EDX,DWORD [264+ECX]
	MOV	EAX,_asm_inthandler21
	MOV	WORD [264+ECX],AX
	MOV	BYTE [4+EDX],0
	MOV	BYTE [5+EDX],-114
	SAR	EAX,16
	MOV	ECX,DWORD [_idt]
	MOV	WORD [6+EDX],AX
	MOV	EAX,_asm_inthandler2c
	MOV	WORD [2+EDX],16
	MOV	WORD [352+ECX],AX
	LEA	EDX,DWORD [352+ECX]
	SAR	EAX,16
	MOV	BYTE [4+EDX],0
	MOV	BYTE [5+EDX],-114
	MOV	WORD [6+EDX],AX
	MOV	ECX,DWORD [_idt]
	MOV	WORD [2+EDX],16
	MOV	EAX,_asm_inthandler27
	LEA	EDX,DWORD [312+ECX]
	MOV	WORD [312+ECX],AX
	SAR	EAX,16
	MOV	WORD [6+EDX],AX
	MOV	WORD [2+EDX],16
	MOV	BYTE [4+EDX],0
	MOV	BYTE [5+EDX],-114
	PUSH	EAX
	PUSH	EAX
	PUSH	2553856
	PUSH	2047
	CALL	_load_idtr
	LEAVE
	RET
	GLOBAL	_set_segmdesc
_set_segmdesc:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	ESI
	PUSH	EBX
	MOV	EDX,DWORD [12+EBP]
	MOV	ESI,DWORD [16+EBP]
	MOV	EBX,DWORD [8+EBP]
	MOV	ECX,DWORD [20+EBP]
	CMP	EDX,1048575
	JBE	L28
	SHR	EDX,3
	OR	ECX,32768
L28:
	MOV	WORD [EBX],DX
	MOV	EAX,ESI
	SAR	EAX,16
	MOV	BYTE [5+EBX],CL
	SAR	ECX,8
	MOV	BYTE [4+EBX],AL
	SHR	EDX,16
	MOV	AL,CL
	MOV	WORD [2+EBX],SI
	AND	EDX,15
	AND	EAX,-16
	OR	EDX,EAX
	MOV	BYTE [6+EBX],DL
	MOV	EDX,ESI
	SAR	EDX,24
	MOV	BYTE [7+EBX],DL
	POP	EBX
	POP	ESI
	POP	EBP
	RET
	GLOBAL	_set_gatedesc
_set_gatedesc:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	EBX
	MOV	EDX,DWORD [8+EBP]
	MOV	EAX,DWORD [16+EBP]
	MOV	EBX,DWORD [20+EBP]
	MOV	ECX,DWORD [12+EBP]
	MOV	WORD [2+EDX],AX
	MOV	BYTE [5+EDX],BL
	MOV	WORD [EDX],CX
	MOV	EAX,EBX
	SAR	EAX,8
	SAR	ECX,16
	MOV	BYTE [4+EDX],AL
	MOV	WORD [6+EDX],CX
	POP	EBX
	POP	EBP
	RET
