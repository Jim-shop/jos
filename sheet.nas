[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[OPTIMIZE 1]
[OPTION 1]
[BITS 32]
	EXTERN	_memman_alloc_4k
	EXTERN	_memman_free_4k
[FILE "sheet.c"]
[SECTION .text]
	GLOBAL	_sheet_updown
_sheet_updown:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	EDI
	PUSH	ESI
	PUSH	EBX
	SUB	ESP,12
	MOV	EDI,DWORD [8+EBP]
	MOV	EBX,DWORD [12+EBP]
	MOV	ESI,DWORD [EDI]
	MOV	EAX,DWORD [28+EDI]
	MOV	DWORD [-16+EBP],EAX
	MOV	EAX,DWORD [16+ESI]
	INC	EAX
	CMP	EBX,EAX
	JLE	L2
	MOV	EBX,EAX
L2:
	CMP	EBX,-2
	SETG	DL
	AND	EDX,255
	DEC	EDX
	OR	EBX,EDX
	MOV	DWORD [28+EDI],EBX
	CMP	DWORD [-16+EBP],EBX
	JLE	L4
	TEST	EBX,EBX
	JS	L5
	MOV	ECX,DWORD [-16+EBP]
L10:
	MOV	EAX,DWORD [16+ESI+ECX*4]
	MOV	DWORD [20+ESI+ECX*4],EAX
	MOV	DWORD [28+EAX],ECX
	DEC	ECX
	CMP	ECX,EBX
	JG	L10
	MOV	DWORD [20+ESI+EBX*4],EDI
	PUSH	EAX
	INC	EBX
	PUSH	EAX
	PUSH	EBX
	MOV	EAX,DWORD [12+EDI]
	MOV	EDX,DWORD [20+EDI]
	ADD	EAX,EDX
	PUSH	EAX
	MOV	EAX,DWORD [8+EDI]
	MOV	ECX,DWORD [16+EDI]
	ADD	EAX,ECX
	PUSH	EAX
	MOV	EAX,DWORD [20+EDI]
	PUSH	EAX
	MOV	EAX,DWORD [16+EDI]
	PUSH	EAX
	PUSH	ESI
	CALL	_sheet_refreshmap
	MOV	EAX,DWORD [-16+EBP]
	ADD	ESP,28
	PUSH	EAX
L40:
	PUSH	EBX
L39:
	MOV	EAX,DWORD [12+EDI]
	MOV	ECX,DWORD [20+EDI]
	ADD	EAX,ECX
	PUSH	EAX
	MOV	EAX,DWORD [8+EDI]
	MOV	EBX,DWORD [16+EDI]
	ADD	EAX,EBX
	PUSH	EAX
	MOV	EAX,DWORD [20+EDI]
	PUSH	EAX
	MOV	EAX,DWORD [16+EDI]
	PUSH	EAX
	PUSH	ESI
	CALL	_sheet_refreshsub
	ADD	ESP,32
L1:
	LEA	ESP,DWORD [-12+EBP]
	POP	EBX
	POP	ESI
	POP	EDI
	POP	EBP
	RET
L5:
	MOV	EAX,DWORD [16+ESI]
	CMP	EAX,DWORD [-16+EBP]
	JLE	L12
	MOV	ECX,DWORD [-16+EBP]
	CMP	ECX,EAX
	JGE	L12
L17:
	MOV	EAX,DWORD [24+ESI+ECX*4]
	MOV	DWORD [20+ESI+ECX*4],EAX
	MOV	DWORD [28+EAX],ECX
	INC	ECX
	MOV	EAX,DWORD [16+ESI]
	CMP	ECX,EAX
	JL	L17
L12:
	DEC	EAX
	MOV	DWORD [16+ESI],EAX
	PUSH	EBX
	PUSH	EBX
	PUSH	0
	MOV	EAX,DWORD [12+EDI]
	MOV	EDX,DWORD [20+EDI]
	ADD	EAX,EDX
	PUSH	EAX
	MOV	EAX,DWORD [8+EDI]
	MOV	EBX,DWORD [16+EDI]
	ADD	EAX,EBX
	PUSH	EAX
	MOV	EAX,DWORD [20+EDI]
	PUSH	EAX
	MOV	EAX,DWORD [16+EDI]
	PUSH	EAX
	PUSH	ESI
	CALL	_sheet_refreshmap
	MOV	EAX,DWORD [-16+EBP]
	ADD	ESP,28
	DEC	EAX
	PUSH	EAX
	PUSH	0
	JMP	L39
L4:
	CMP	DWORD [-16+EBP],EBX
	JGE	L1
	MOV	EAX,DWORD [-16+EBP]
	TEST	EAX,EAX
	JS	L20
	MOV	ECX,DWORD [-16+EBP]
	CMP	ECX,EBX
	JGE	L36
L25:
	MOV	EAX,DWORD [24+ESI+ECX*4]
	MOV	DWORD [20+ESI+ECX*4],EAX
	MOV	DWORD [28+EAX],ECX
	INC	ECX
	CMP	ECX,EBX
	JL	L25
L36:
	MOV	DWORD [20+ESI+EBX*4],EDI
L26:
	PUSH	EAX
	PUSH	EAX
	PUSH	EBX
	MOV	EAX,DWORD [12+EDI]
	MOV	EDX,DWORD [20+EDI]
	ADD	EAX,EDX
	PUSH	EAX
	MOV	EAX,DWORD [8+EDI]
	MOV	ECX,DWORD [16+EDI]
	ADD	EAX,ECX
	PUSH	EAX
	MOV	EAX,DWORD [20+EDI]
	PUSH	EAX
	MOV	EAX,DWORD [16+EDI]
	PUSH	EAX
	PUSH	ESI
	CALL	_sheet_refreshmap
	ADD	ESP,28
	PUSH	EBX
	JMP	L40
L20:
	MOV	EAX,DWORD [16+ESI]
	MOV	ECX,EAX
	CMP	EAX,EBX
	JL	L38
L31:
	MOV	EDX,DWORD [20+ESI+ECX*4]
	LEA	EAX,DWORD [1+ECX]
	MOV	DWORD [24+ESI+ECX*4],EDX
	DEC	ECX
	MOV	DWORD [28+EDX],EAX
	CMP	ECX,EBX
	JGE	L31
	MOV	EAX,DWORD [16+ESI]
L38:
	INC	EAX
	MOV	DWORD [20+ESI+EBX*4],EDI
	MOV	DWORD [16+ESI],EAX
	JMP	L26
	GLOBAL	_sheet_refreshsub
_sheet_refreshsub:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	EDI
	PUSH	ESI
	PUSH	EBX
	SUB	ESP,36
	MOV	EAX,DWORD [8+EBP]
	MOV	EBX,DWORD [8+EBP]
	MOV	EDX,DWORD [12+EBP]
	MOV	ECX,DWORD [16+EBP]
	MOV	EAX,DWORD [EAX]
	MOV	DWORD [-32+EBP],EAX
	MOV	EAX,DWORD [4+EBX]
	MOV	DWORD [-36+EBP],EAX
	MOV	EAX,DWORD [12+EBP]
	SHR	EAX,31
	DEC	EAX
	AND	EDX,EAX
	MOV	EAX,DWORD [16+EBP]
	SHR	EAX,31
	MOV	DWORD [12+EBP],EDX
	MOV	EDX,DWORD [8+EBP]
	DEC	EAX
	AND	ECX,EAX
	MOV	EAX,DWORD [8+EDX]
	MOV	DWORD [16+EBP],ECX
	CMP	DWORD [20+EBP],EAX
	JLE	L44
	MOV	DWORD [20+EBP],EAX
L44:
	MOV	ESI,DWORD [8+EBP]
	MOV	EAX,DWORD [12+ESI]
	CMP	DWORD [24+EBP],EAX
	JLE	L45
	MOV	DWORD [24+EBP],EAX
L45:
	MOV	EDI,DWORD [28+EBP]
	MOV	EAX,DWORD [32+EBP]
	MOV	DWORD [-16+EBP],EDI
	CMP	EDI,EAX
	JG	L67
L65:
	MOV	EDI,DWORD [8+EBP]
	MOV	ECX,DWORD [-16+EBP]
	MOV	EBX,DWORD [20+EDI+ECX*4]
	MOV	EDX,EBX
	SUB	EDX,EDI
	MOV	EAX,DWORD [4+EBX]
	SUB	EDX,1044
	MOV	DWORD [-40+EBP],EAX
	SAR	EDX,2
	LEA	ESI,DWORD [0+EDX*8]
	SUB	ESI,EDX
	MOV	EAX,ESI
	SAL	EAX,6
	ADD	ESI,EAX
	MOV	EAX,DWORD [16+EBX]
	LEA	ECX,DWORD [EDX+ESI*8]
	MOV	ESI,DWORD [20+EBP]
	SAL	ECX,3
	SUB	ESI,EAX
	SUB	DL,CL
	MOV	DWORD [-24+EBP],ESI
	MOV	ECX,DWORD [12+EBP]
	MOV	BYTE [-41+EBP],DL
	SUB	ECX,EAX
	MOV	EDX,DWORD [20+EBX]
	MOV	EAX,DWORD [24+EBP]
	MOV	DWORD [-20+EBP],ECX
	SUB	EAX,EDX
	MOV	EDI,DWORD [-20+EBP]
	MOV	DWORD [-28+EBP],EAX
	MOV	ECX,DWORD [16+EBP]
	MOV	EAX,DWORD [-20+EBP]
	SUB	ECX,EDX
	SHR	EAX,31
	DEC	EAX
	AND	EDI,EAX
	MOV	EAX,DWORD [8+EBX]
	MOV	DWORD [-20+EBP],EDI
	MOV	EDI,ECX
	SHR	EDI,31
	DEC	EDI
	AND	ECX,EDI
	CMP	ESI,EAX
	JLE	L52
	MOV	DWORD [-24+EBP],EAX
L52:
	MOV	EAX,DWORD [12+EBX]
	CMP	DWORD [-28+EBP],EAX
	JLE	L53
	MOV	DWORD [-28+EBP],EAX
L53:
	MOV	EAX,DWORD [-28+EBP]
	MOV	DWORD [-48+EBP],ECX
	CMP	ECX,EAX
	JGE	L69
L64:
	MOV	EAX,DWORD [-48+EBP]
	MOV	ECX,DWORD [-20+EBP]
	CMP	ECX,DWORD [-24+EBP]
	LEA	EDI,DWORD [EAX+EDX*1]
	JGE	L71
L63:
	MOV	ESI,DWORD [8+EBP]
	MOV	EAX,EDI
	MOV	EDX,DWORD [16+EBX]
	IMUL	EAX,DWORD [8+ESI]
	ADD	EDX,ECX
	MOV	ESI,DWORD [-36+EBP]
	LEA	EDX,DWORD [EDX+EAX*1]
	MOV	AL,BYTE [-41+EBP]
	CMP	BYTE [EDX+ESI*1],AL
	JE	L73
L60:
	INC	ECX
	CMP	ECX,DWORD [-24+EBP]
	JL	L63
L71:
	MOV	EAX,DWORD [-48+EBP]
	INC	EAX
	MOV	DWORD [-48+EBP],EAX
	MOV	EAX,DWORD [-28+EBP]
	CMP	DWORD [-48+EBP],EAX
	JGE	L69
	MOV	EDX,DWORD [20+EBX]
	JMP	L64
L69:
	MOV	EAX,DWORD [-16+EBP]
	MOV	EBX,DWORD [32+EBP]
	INC	EAX
	MOV	DWORD [-16+EBP],EAX
	CMP	DWORD [-16+EBP],EBX
	JLE	L65
L67:
	ADD	ESP,36
	POP	EBX
	POP	ESI
	POP	EDI
	POP	EBP
	RET
L73:
	MOV	ESI,DWORD [8+EBX]
	MOV	EAX,DWORD [-48+EBP]
	IMUL	EAX,ESI
	MOV	ESI,DWORD [-40+EBP]
	ADD	EAX,ECX
	MOV	AL,BYTE [EAX+ESI*1]
	MOV	ESI,DWORD [-32+EBP]
	MOV	BYTE [EDX+ESI*1],AL
	JMP	L60
	GLOBAL	_sheet_refreshmap
_sheet_refreshmap:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	EDI
	PUSH	ESI
	PUSH	EBX
	SUB	ESP,36
	MOV	EAX,DWORD [8+EBP]
	MOV	EDX,DWORD [12+EBP]
	MOV	ECX,DWORD [16+EBP]
	MOV	EAX,DWORD [4+EAX]
	MOV	DWORD [-32+EBP],EAX
	MOV	EAX,DWORD [12+EBP]
	SHR	EAX,31
	DEC	EAX
	AND	EDX,EAX
	MOV	EAX,DWORD [16+EBP]
	SHR	EAX,31
	MOV	DWORD [12+EBP],EDX
	DEC	EAX
	AND	ECX,EAX
	MOV	DWORD [16+EBP],ECX
	MOV	ECX,DWORD [8+EBP]
	MOV	EAX,DWORD [8+ECX]
	CMP	DWORD [20+EBP],EAX
	JLE	L77
	MOV	DWORD [20+EBP],EAX
L77:
	MOV	EBX,DWORD [8+EBP]
	MOV	EAX,DWORD [12+EBX]
	CMP	DWORD [24+EBP],EAX
	JLE	L78
	MOV	DWORD [24+EBP],EAX
L78:
	MOV	EAX,DWORD [8+EBP]
	MOV	ESI,DWORD [28+EBP]
	MOV	DWORD [-16+EBP],ESI
	MOV	EAX,DWORD [16+EAX]
	MOV	DWORD [-44+EBP],EAX
	CMP	ESI,EAX
	JG	L100
L98:
	MOV	EAX,DWORD [8+EBP]
	MOV	ECX,DWORD [-16+EBP]
	MOV	EBX,DWORD [20+EAX+ECX*4]
	MOV	EDX,EBX
	SUB	EDX,EAX
	MOV	EAX,DWORD [16+EBX]
	SUB	EDX,1044
	SAR	EDX,2
	LEA	ESI,DWORD [0+EDX*8]
	SUB	ESI,EDX
	MOV	EDI,ESI
	SAL	EDI,6
	ADD	ESI,EDI
	LEA	ECX,DWORD [EDX+ESI*8]
	MOV	ESI,DWORD [20+EBP]
	SAL	ECX,3
	SUB	ESI,EAX
	SUB	DL,CL
	MOV	DWORD [-48+EBP],ESI
	MOV	ECX,DWORD [12+EBP]
	MOV	BYTE [-37+EBP],DL
	SUB	ECX,EAX
	MOV	EDX,DWORD [4+EBX]
	MOV	DWORD [-24+EBP],ECX
	MOV	EAX,DWORD [24+EBP]
	MOV	ECX,DWORD [20+EBX]
	MOV	EDI,DWORD [-24+EBP]
	SUB	EAX,ECX
	MOV	DWORD [-36+EBP],EDX
	MOV	DWORD [-28+EBP],EAX
	MOV	EDX,DWORD [16+EBP]
	MOV	EAX,DWORD [-24+EBP]
	SUB	EDX,ECX
	SHR	EAX,31
	DEC	EAX
	AND	EDI,EAX
	MOV	EAX,DWORD [8+EBX]
	MOV	DWORD [-24+EBP],EDI
	MOV	EDI,EDX
	SHR	EDI,31
	DEC	EDI
	AND	EDX,EDI
	CMP	ESI,EAX
	JLE	L85
	MOV	DWORD [-48+EBP],EAX
L85:
	MOV	EAX,DWORD [12+EBX]
	CMP	DWORD [-28+EBP],EAX
	JLE	L86
	MOV	DWORD [-28+EBP],EAX
L86:
	MOV	ESI,EDX
	CMP	EDX,DWORD [-28+EBP]
	JGE	L102
L97:
	ADD	ECX,ESI
	MOV	DWORD [-20+EBP],ECX
	MOV	ECX,DWORD [-24+EBP]
	CMP	ECX,DWORD [-48+EBP]
	JGE	L104
L96:
	MOV	EDI,DWORD [8+EBX]
	MOV	EAX,ESI
	IMUL	EAX,EDI
	MOV	EDI,DWORD [-36+EBP]
	ADD	EAX,ECX
	MOV	EDX,DWORD [16+EBX]
	MOV	AL,BYTE [EAX+EDI*1]
	ADD	EDX,ECX
	AND	EAX,255
	CMP	EAX,DWORD [24+EBX]
	JE	L93
	MOV	EDI,DWORD [8+EBP]
	MOV	EAX,DWORD [-20+EBP]
	IMUL	EAX,DWORD [8+EDI]
	MOV	EDI,DWORD [-32+EBP]
	ADD	EAX,EDX
	MOV	DL,BYTE [-37+EBP]
	MOV	BYTE [EAX+EDI*1],DL
L93:
	INC	ECX
	CMP	ECX,DWORD [-48+EBP]
	JL	L96
L104:
	INC	ESI
	CMP	ESI,DWORD [-28+EBP]
	JGE	L106
	MOV	ECX,DWORD [20+EBX]
	JMP	L97
L106:
	MOV	EAX,DWORD [8+EBP]
	MOV	EAX,DWORD [16+EAX]
	MOV	DWORD [-44+EBP],EAX
L102:
	MOV	EDX,DWORD [-16+EBP]
	MOV	EBX,DWORD [-44+EBP]
	INC	EDX
	MOV	DWORD [-16+EBP],EDX
	CMP	DWORD [-16+EBP],EBX
	JLE	L98
L100:
	ADD	ESP,36
	POP	EBX
	POP	ESI
	POP	EDI
	POP	EBP
	RET
	GLOBAL	_shtctl_init
_shtctl_init:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	EDI
	PUSH	ESI
	PUSH	EBX
	SUB	ESP,20
	MOV	ESI,DWORD [8+EBP]
	MOV	EDI,DWORD [16+EBP]
	PUSH	10260
	PUSH	ESI
	CALL	_memman_alloc_4k
	ADD	ESP,16
	MOV	EBX,EAX
	XOR	EAX,EAX
	TEST	EBX,EBX
	JE	L107
	PUSH	EAX
	PUSH	EAX
	MOV	EAX,DWORD [20+EBP]
	IMUL	EAX,EDI
	PUSH	EAX
	PUSH	ESI
	CALL	_memman_alloc_4k
	ADD	ESP,16
	TEST	EAX,EAX
	MOV	DWORD [4+EBX],EAX
	JE	L118
	MOV	EAX,DWORD [12+EBP]
	MOV	DWORD [8+EBX],EDI
	MOV	DWORD [EBX],EAX
	MOV	EDX,255
	MOV	EAX,DWORD [20+EBP]
	MOV	DWORD [12+EBX],EAX
	MOV	EAX,EBX
	MOV	DWORD [16+EBX],-1
L114:
	MOV	DWORD [1076+EAX],0
	MOV	DWORD [1044+EAX],EBX
	ADD	EAX,36
	DEC	EDX
	JNS	L114
L117:
	MOV	EAX,EBX
L107:
	LEA	ESP,DWORD [-12+EBP]
	POP	EBX
	POP	ESI
	POP	EDI
	POP	EBP
	RET
L118:
	PUSH	EDI
	PUSH	10260
	PUSH	EBX
	PUSH	ESI
	CALL	_memman_free_4k
	JMP	L117
	GLOBAL	_sheet_alloc
_sheet_alloc:
	PUSH	EBP
	XOR	EDX,EDX
	MOV	EBP,ESP
	MOV	EAX,DWORD [8+EBP]
	ADD	EAX,1044
L125:
	MOV	ECX,DWORD [32+EAX]
	TEST	ECX,ECX
	JE	L128
	INC	EDX
	ADD	EAX,36
	CMP	EDX,255
	JLE	L125
	XOR	EAX,EAX
L119:
	POP	EBP
	RET
L128:
	MOV	DWORD [32+EAX],1
	MOV	DWORD [28+EAX],-1
	JMP	L119
	GLOBAL	_sheet_setbuf
_sheet_setbuf:
	PUSH	EBP
	MOV	EBP,ESP
	MOV	ECX,DWORD [8+EBP]
	MOV	EAX,DWORD [12+EBP]
	MOV	DWORD [4+ECX],EAX
	MOV	EAX,DWORD [16+EBP]
	MOV	DWORD [8+ECX],EAX
	MOV	EAX,DWORD [20+EBP]
	MOV	DWORD [12+ECX],EAX
	MOV	EAX,DWORD [24+EBP]
	MOV	DWORD [24+ECX],EAX
	POP	EBP
	RET
	GLOBAL	_sheet_refresh
_sheet_refresh:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	EAX
	PUSH	EAX
	MOV	EDX,DWORD [8+EBP]
	MOV	EAX,DWORD [28+EDX]
	TEST	EAX,EAX
	JS	L130
	PUSH	ECX
	PUSH	EAX
	MOV	EAX,DWORD [28+EDX]
	PUSH	EAX
	MOV	EAX,DWORD [24+EBP]
	MOV	ECX,DWORD [20+EDX]
	ADD	EAX,ECX
	PUSH	EAX
	MOV	EAX,DWORD [20+EBP]
	MOV	ECX,DWORD [16+EDX]
	ADD	EAX,ECX
	PUSH	EAX
	MOV	EAX,DWORD [16+EBP]
	MOV	ECX,DWORD [20+EDX]
	ADD	EAX,ECX
	PUSH	EAX
	MOV	EAX,DWORD [12+EBP]
	MOV	ECX,DWORD [16+EDX]
	ADD	EAX,ECX
	PUSH	EAX
	MOV	EAX,DWORD [EDX]
	PUSH	EAX
	CALL	_sheet_refreshsub
	ADD	ESP,32
L130:
	MOV	ESP,EBP
	POP	EBP
	RET
	GLOBAL	_sheet_slide
_sheet_slide:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	EDI
	PUSH	ESI
	PUSH	EBX
	SUB	ESP,12
	MOV	EBX,DWORD [8+EBP]
	MOV	EAX,DWORD [12+EBP]
	MOV	EDI,DWORD [16+EBX]
	MOV	ESI,DWORD [20+EBX]
	MOV	DWORD [16+EBX],EAX
	MOV	EAX,DWORD [16+EBP]
	MOV	DWORD [20+EBX],EAX
	MOV	EAX,DWORD [28+EBX]
	TEST	EAX,EAX
	JS	L132
	MOV	EDX,ESI
	PUSH	EAX
	MOV	ECX,EDI
	PUSH	EAX
	PUSH	0
	MOV	EAX,DWORD [12+EBX]
	ADD	EDX,EAX
	PUSH	EDX
	MOV	EAX,DWORD [8+EBX]
	ADD	ECX,EAX
	PUSH	ECX
	PUSH	ESI
	PUSH	EDI
	MOV	EAX,DWORD [EBX]
	PUSH	EAX
	CALL	_sheet_refreshmap
	MOV	EAX,DWORD [28+EBX]
	ADD	ESP,24
	PUSH	EAX
	MOV	EAX,DWORD [16+EBP]
	MOV	EDX,DWORD [12+EBX]
	ADD	EAX,EDX
	PUSH	EAX
	MOV	EAX,DWORD [12+EBP]
	MOV	ECX,DWORD [8+EBX]
	ADD	EAX,ECX
	PUSH	EAX
	MOV	EAX,DWORD [16+EBP]
	PUSH	EAX
	MOV	EAX,DWORD [12+EBP]
	PUSH	EAX
	MOV	EAX,DWORD [EBX]
	PUSH	EAX
	CALL	_sheet_refreshmap
	MOV	ECX,EDI
	ADD	ESP,28
	MOV	EAX,DWORD [28+EBX]
	DEC	EAX
	MOV	EDX,ESI
	PUSH	EAX
	PUSH	0
	MOV	EAX,DWORD [12+EBX]
	ADD	EDX,EAX
	PUSH	EDX
	MOV	EAX,DWORD [8+EBX]
	ADD	ECX,EAX
	PUSH	ECX
	PUSH	ESI
	PUSH	EDI
	MOV	EAX,DWORD [EBX]
	PUSH	EAX
	CALL	_sheet_refreshsub
	MOV	ECX,DWORD [28+EBX]
	ADD	ESP,28
	PUSH	ECX
	MOV	EAX,DWORD [28+EBX]
	PUSH	EAX
	MOV	EAX,DWORD [16+EBP]
	MOV	EDX,DWORD [12+EBX]
	ADD	EAX,EDX
	PUSH	EAX
	MOV	EAX,DWORD [12+EBP]
	MOV	ECX,DWORD [8+EBX]
	ADD	EAX,ECX
	PUSH	EAX
	MOV	EAX,DWORD [16+EBP]
	PUSH	EAX
	MOV	EAX,DWORD [12+EBP]
	PUSH	EAX
	MOV	EAX,DWORD [EBX]
	PUSH	EAX
	CALL	_sheet_refreshsub
	ADD	ESP,32
L132:
	LEA	ESP,DWORD [-12+EBP]
	POP	EBX
	POP	ESI
	POP	EDI
	POP	EBP
	RET
	GLOBAL	_sheet_free
_sheet_free:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	EBX
	PUSH	EAX
	MOV	EBX,DWORD [8+EBP]
	MOV	EAX,DWORD [28+EBX]
	TEST	EAX,EAX
	JS	L135
	PUSH	EAX
	PUSH	EAX
	PUSH	-1
	PUSH	EBX
	CALL	_sheet_updown
	ADD	ESP,16
L135:
	MOV	DWORD [32+EBX],0
	MOV	EBX,DWORD [-4+EBP]
	MOV	ESP,EBP
	POP	EBP
	RET
