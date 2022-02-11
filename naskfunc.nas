; naskfunc

[FORMAT "WCOFF"]				; 制作目标文件的模式
[INSTRSET "i486p"]				; 使用到486为止的指令
[BITS 32]						; 制作32位模式用的机器语言

; 制作目标文件的信息

[FILE "naskfunc.nas"]			; 源文件名信息

		; 程序中包含的函数名
		; GLOBAL 此文件中
		; EXTERN 来自其他文件
		GLOBAL	_io_hlt, _io_cli, _io_sti, _io_stihlt
		GLOBAL	_io_in8,  _io_in16,  _io_in32
		GLOBAL	_io_out8, _io_out16, _io_out32
		GLOBAL	_io_load_eflags, _io_store_eflags
		GLOBAL	_load_gdtr, _load_idtr
		GLOBAL	_asm_inthandler21, _asm_inthandler27, _asm_inthandler2c
		EXTERN	_inthandler21, _inthandler27, _inthandler2c


; 以下是实际的函数

[SECTION .text]		; 目标文件中写了这些后再写程序

; [ESP+4]第一个参数，[ESP+8]第二个参数，etc
; 汇编可自由使用的寄存器 EAX, ECX, EDX
; IN 	EAX, DX		; 从DX所指的端口取数据进寄存器EAX
; OUT 	DX, AL		; 向DX所指的端口发送AL中的数据

_io_hlt:	; void io_hlt(void);
		HLT
		RET

_io_cli:	; void io_cli(void);
		CLI
		RET

_io_sti:	; void io_sti(void);
		STI
		RET

_io_stihlt:	; void io_stihlt(void);
		STI
		HLT
		RET

_io_in8:	; int io_in8(int port);
		MOV		EDX, [ESP+4]		
		MOV		EAX, 0			; 通过操作EAX来操作AL，提高指令效率
		IN		AL, DX
		RET

_io_in16:	; int io_in16(int port);
		MOV		EDX, [ESP+4]
		MOV		EAX, 0			; 通过操作EAX来操作AX，提高指令效率
		IN		AX, DX
		RET

_io_in32:	; int io_in32(int port);
		MOV		EDX, [ESP+4]
		IN		EAX, DX
		RET

_io_out8:	; void io_out8(int port, int data);
		MOV		EDX, [ESP+4]
		MOV		EAX, [ESP+8]
		OUT		DX, AL
		RET

_io_out16:	; void io_out16(int port, int data);
		MOV		EDX, [ESP+4]
		MOV		EAX, [ESP+8]
		OUT		DX, AX
		RET

_io_out32:	; void io_out32(int port, int data);
		MOV		EDX, [ESP+4]
		MOV		EAX, [ESP+8]
		OUT		DX, EAX
		RET

_io_load_eflags:	; int io_load_eflags(void);
		PUSHFD					; 压栈EFLAGS
		POP		EAX				; 没有MOV EAX, EFLAGS指令，曲线救国
		RET

_io_store_eflags:	; void io_store_eflags(int eflags);
		MOV		EAX,[ESP+4]
		PUSH	EAX
		POPFD					; 出栈到EFLAGS
		RET

_load_gdtr:		; void load_gdtr(int limit, int addr);
		; GDTR是48位寄存器，只能使用 LGDT 指令从一个内存地址赋值
		; 该寄存器的低16位是段上限（等于GDT的有效字节数-1）
		; 高32位表示GDT的开始地址。
		; 假设传入的参数limit=0x0000ffff, add=0x00270000
		; 其在内存中的分布就是：（注意x86是小端序，低字节存在低内存）
		; [ESP] +4 +5 +6 +7 +8 +9 +10 +11
		;       FF FF 00 00 00 00 27  00  
		; 想要让 LGDT 读到 [FF FF 00 00 27 00]的形式
		; 则可以用16位寄存器把[ESP+4]处两个字节拷到[ESP+6]处两个字节上
		; 然后让 LGDT 读[ESP+6]处的6个字节
		MOV		AX,[ESP+4]		
		MOV		[ESP+6],AX
		LGDT	[ESP+6]			
		RET

_load_idtr:		; void load_idtr(int limit, int addr);
		MOV		AX,[ESP+4]	
		MOV		[ESP+6],AX
		LIDT	[ESP+6]			; 与GDT表设置类似，用LIDT设置IDT表
		RET

_asm_inthandler21:
		; PUSH EAX 相当于:
		; 	ADD ESP, -4
		; 	MOV [SS:ESP], EAX
		; POP EAX 相当于：
		;	MOV EAX, [SS:ESP]
		; 	ADD ESP, 4
		PUSH	ES				
		PUSH	DS
		PUSHAD					; 相当于PUSH EAX,ECX,EDX,EBX,ESP,ESI,EDI
		MOV		EAX, ESP
		PUSH	EAX				
		; 以上5句保存寄存器的值
		MOV		AX, SS			; SS 栈段寄存器
		MOV		DS, AX
		MOV		ES, AX			
		; 让DS,ES与SS相等，因为C语言默认它们相等
		; 如此设定才能保证C语言代码顺利执行
		; 保存好寄存器的值并配置好环境后就能移交C语言执行了
		CALL	_inthandler21	; CALL 调用函数
		; C语言执行完成返回此处，接下来就还原寄存器的值
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD					; 返回触发中断前位置

_asm_inthandler27:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX, ESP
		PUSH	EAX
		MOV		AX, SS
		MOV		DS, AX
		MOV		ES, AX
		CALL	_inthandler27
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_asm_inthandler2c:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX, ESP
		PUSH	EAX
		MOV		AX, SS
		MOV		DS, AX
		MOV		ES, AX
		CALL	_inthandler2c
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD
