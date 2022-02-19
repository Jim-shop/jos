[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[OPTIMIZE 1]
[OPTION 1]
[BITS 32]
	EXTERN	_io_load_eflags
	EXTERN	_io_store_eflags
	EXTERN	_load_cr0
	EXTERN	_store_cr0
[FILE "memory.c"]
[SECTION .text]
	GLOBAL	_memtest
_memtest:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	ESI
	PUSH	EBX
	XOR	ESI,ESI
	PUSH	ECX
	CALL	_io_load_eflags
	MOV	DWORD [-12+EBP],EAX
	MOV	EAX,DWORD [-12+EBP]
	OR	EAX,262144
	MOV	DWORD [-12+EBP],EAX
	MOV	EAX,DWORD [-12+EBP]
	PUSH	EAX
	CALL	_io_store_eflags
	CALL	_io_load_eflags
	POP	EDX
	MOV	DWORD [-12+EBP],EAX
	MOV	EAX,DWORD [-12+EBP]
	TEST	EAX,262144
	JE	L2
	MOV	ESI,1
L2:
	MOV	EAX,DWORD [-12+EBP]
	AND	EAX,-262145
	MOV	DWORD [-12+EBP],EAX
	MOV	EAX,DWORD [-12+EBP]
	PUSH	EAX
	CALL	_io_store_eflags
	POP	EAX
	MOV	EAX,ESI
	TEST	AL,AL
	JNE	L5
L3:
	PUSH	DWORD [12+EBP]
	PUSH	DWORD [8+EBP]
	CALL	_memtest_sub
	POP	EDX
	MOV	EBX,EAX
	POP	ECX
	MOV	EAX,ESI
	TEST	AL,AL
	JNE	L6
L4:
	LEA	ESP,DWORD [-8+EBP]
	MOV	EAX,EBX
	POP	EBX
	POP	ESI
	POP	EBP
	RET
L6:
	CALL	_load_cr0
	AND	EAX,-1610612737
	PUSH	EAX
	CALL	_store_cr0
	POP	EAX
	JMP	L4
L5:
	CALL	_load_cr0
	OR	EAX,1610612736
	PUSH	EAX
	CALL	_store_cr0
	POP	EBX
	JMP	L3
	GLOBAL	_memtest_sub
_memtest_sub:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	ESI
	PUSH	EBX
	MOV	EDX,DWORD [8+EBP]
	MOV	ESI,DWORD [12+EBP]
	CMP	EDX,ESI
	JA	L9
L15:
	MOV	ECX,DWORD [4092+EDX]
	LEA	EBX,DWORD [4092+EDX]
	MOV	DWORD [4092+EDX],-1437226411
	MOV	EAX,DWORD [4092+EDX]
	NOT	EAX
	MOV	DWORD [4092+EDX],EAX
	MOV	EAX,DWORD [4092+EDX]
	CMP	EAX,1437226410
	JNE	L13
	MOV	EAX,DWORD [4092+EDX]
	NOT	EAX
	MOV	DWORD [4092+EDX],EAX
	MOV	EAX,DWORD [4092+EDX]
	CMP	EAX,-1437226411
	JNE	L13
	MOV	DWORD [4092+EDX],ECX
	ADD	EDX,4096
	CMP	EDX,ESI
	JBE	L15
L9:
	POP	EBX
	MOV	EAX,EDX
	POP	ESI
	POP	EBP
	RET
L13:
	MOV	DWORD [EBX],ECX
	JMP	L9
	GLOBAL	_memman_init
_memman_init:
	PUSH	EBP
	MOV	EBP,ESP
	MOV	EAX,DWORD [8+EBP]
	MOV	DWORD [EAX],0
	MOV	DWORD [4+EAX],0
	MOV	DWORD [8+EAX],0
	MOV	DWORD [12+EAX],0
	POP	EBP
	RET
	GLOBAL	_memman_total
_memman_total:
	PUSH	EBP
	XOR	EAX,EAX
	MOV	EBP,ESP
	XOR	EDX,EDX
	PUSH	EBX
	MOV	EBX,DWORD [8+EBP]
	MOV	ECX,DWORD [EBX]
	CMP	EAX,ECX
	JAE	L25
L23:
	ADD	EAX,DWORD [20+EBX+EDX*8]
	INC	EDX
	CMP	EDX,ECX
	JB	L23
L25:
	POP	EBX
	POP	EBP
	RET
	GLOBAL	_memman_alloc
_memman_alloc:
	PUSH	EBP
	XOR	ECX,ECX
	MOV	EBP,ESP
	PUSH	EDI
	PUSH	ESI
	PUSH	EBX
	MOV	ESI,DWORD [12+EBP]
	MOV	EBX,DWORD [8+EBP]
	MOV	EAX,DWORD [EBX]
	CMP	ECX,EAX
	JAE	L40
L38:
	MOV	EDX,DWORD [20+EBX+ECX*8]
	CMP	EDX,ESI
	JAE	L42
	INC	ECX
	CMP	ECX,EAX
	JB	L38
L40:
	XOR	EAX,EAX
L26:
	POP	EBX
	POP	ESI
	POP	EDI
	POP	EBP
	RET
L42:
	MOV	EDI,DWORD [16+EBX+ECX*8]
	LEA	EAX,DWORD [ESI+EDI*1]
	MOV	DWORD [16+EBX+ECX*8],EAX
	MOV	EAX,EDX
	SUB	EAX,ESI
	MOV	DWORD [20+EBX+ECX*8],EAX
	TEST	EAX,EAX
	JNE	L32
	MOV	EAX,DWORD [EBX]
	DEC	EAX
	MOV	DWORD [EBX],EAX
	CMP	ECX,EAX
	JAE	L32
	MOV	ESI,EAX
L37:
	MOV	EAX,DWORD [24+EBX+ECX*8]
	MOV	EDX,DWORD [28+EBX+ECX*8]
	MOV	DWORD [16+EBX+ECX*8],EAX
	MOV	DWORD [20+EBX+ECX*8],EDX
	INC	ECX
	CMP	ECX,ESI
	JB	L37
L32:
	MOV	EAX,EDI
	JMP	L26
	GLOBAL	_memman_free
_memman_free:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	EDI
	PUSH	ESI
	MOV	ESI,DWORD [8+EBP]
	PUSH	EBX
	XOR	EBX,EBX
	MOV	EDI,DWORD [ESI]
	CMP	EBX,EDI
	JGE	L45
L49:
	MOV	EAX,DWORD [12+EBP]
	CMP	DWORD [16+ESI+EBX*8],EAX
	JA	L45
	INC	EBX
	CMP	EBX,EDI
	JL	L49
L45:
	TEST	EBX,EBX
	JLE	L50
	MOV	EDX,DWORD [12+ESI+EBX*8]
	MOV	EAX,DWORD [8+ESI+EBX*8]
	ADD	EAX,EDX
	CMP	EAX,DWORD [12+EBP]
	JE	L73
L50:
	CMP	EBX,EDI
	JGE	L59
	MOV	EAX,DWORD [12+EBP]
	ADD	EAX,DWORD [16+EBP]
	CMP	EAX,DWORD [16+ESI+EBX*8]
	JE	L74
L59:
	CMP	EDI,4089
	JG	L61
	MOV	ECX,EDI
	CMP	EDI,EBX
	JLE	L71
L66:
	MOV	EAX,DWORD [8+ESI+ECX*8]
	MOV	EDX,DWORD [12+ESI+ECX*8]
	MOV	DWORD [16+ESI+ECX*8],EAX
	MOV	DWORD [20+ESI+ECX*8],EDX
	DEC	ECX
	CMP	ECX,EBX
	JG	L66
L71:
	LEA	EAX,DWORD [1+EDI]
	MOV	DWORD [ESI],EAX
	CMP	DWORD [4+ESI],EAX
	JGE	L67
	MOV	DWORD [4+ESI],EAX
L67:
	MOV	EAX,DWORD [12+EBP]
	MOV	DWORD [16+ESI+EBX*8],EAX
	MOV	EAX,DWORD [16+EBP]
	MOV	DWORD [20+ESI+EBX*8],EAX
L72:
	XOR	EAX,EAX
L43:
	POP	EBX
	POP	ESI
	POP	EDI
	POP	EBP
	RET
L61:
	MOV	EAX,DWORD [16+EBP]
	INC	DWORD [12+ESI]
	ADD	DWORD [8+ESI],EAX
	OR	EAX,-1
	JMP	L43
L74:
	MOV	EAX,DWORD [12+EBP]
	MOV	DWORD [16+ESI+EBX*8],EAX
	MOV	EAX,DWORD [16+EBP]
	ADD	DWORD [20+ESI+EBX*8],EAX
	JMP	L72
L73:
	ADD	EDX,DWORD [16+EBP]
	MOV	DWORD [12+ESI+EBX*8],EDX
	CMP	EBX,DWORD [ESI]
	JGE	L72
	MOV	EAX,DWORD [12+EBP]
	ADD	EAX,DWORD [16+EBP]
	CMP	EAX,DWORD [16+ESI+EBX*8]
	JNE	L72
	ADD	EDX,DWORD [20+ESI+EBX*8]
	MOV	DWORD [12+ESI+EBX*8],EDX
	MOV	EAX,DWORD [ESI]
	DEC	EAX
	MOV	DWORD [ESI],EAX
	CMP	EBX,EAX
	JGE	L72
	MOV	ECX,EAX
L58:
	MOV	EAX,DWORD [24+ESI+EBX*8]
	MOV	EDX,DWORD [28+ESI+EBX*8]
	MOV	DWORD [16+ESI+EBX*8],EAX
	MOV	DWORD [20+ESI+EBX*8],EDX
	INC	EBX
	CMP	EBX,ECX
	JL	L58
	JMP	L72
	GLOBAL	_memman_alloc_4k
_memman_alloc_4k:
	PUSH	EBP
	MOV	EBP,ESP
	MOV	EAX,DWORD [12+EBP]
	ADD	EAX,4095
	AND	EAX,-4096
	MOV	DWORD [12+EBP],EAX
	POP	EBP
	JMP	_memman_alloc
	GLOBAL	_memman_free_4k
_memman_free_4k:
	PUSH	EBP
	MOV	EBP,ESP
	MOV	EAX,DWORD [16+EBP]
	ADD	EAX,4095
	AND	EAX,-4096
	MOV	DWORD [16+EBP],EAX
	POP	EBP
	JMP	_memman_free
