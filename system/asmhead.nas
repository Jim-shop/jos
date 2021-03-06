; jos-asmhead

[INSTRSET "i486p"]			; 知会汇编器想要使用486指令（32位）

; 显示信息参数设定
	; 画面模式如：
	; 0x101: 640x480x8bit
	; 0x103: 800x600x8bit
	; 0x105: 1024x768x8bit
	; 0x107: 1280x1024x8bit (qemu无法指定)
VBEMODE EQU		0x105

; 信息存取地址
CYLS    EQU     0x0ff0      ; [BYTE]读入的柱面数
LEDS    EQU     0x0ff1      ; [BYTE]键盘LED状态(4，5，6位依次是ScrollLock, NumLock, CapsLock)
VMODE   EQU     0x0ff2      ; [BYTE]颜色位数
                            ; 配合C语言结构体对齐，此处内存留空
SCRNX   EQU     0x0ff4      ; [WORD]分辨率x
SCRNY   EQU     0x0ff6      ; [WORD]分辨率y
VRAM    EQU     0x0ff8      ; [DWORD]图像缓冲区的开始地址
; 各种文件存放地址
BOTPAK	EQU		0x00280000	; bootpack加载到的位置
DSKCAC	EQU		0x00100000	; 磁盘缓存的去向位置（数据复制过去只是为了方便管理）
DSKCAC0	EQU		0x00008000	; 磁盘缓存的来源位置（实模式）


    ORG     0xc200          ; 知会汇编程序 程序被装载在此内存地址

; 确认VBE是否存在
	MOV		AX, 0x9000
	MOV 	ES, AX
	MOV		DI, 0			; 显卡能利用的VBE信息要写入[ES:DI]开始的512字节中
	MOV		AX, 0x4f00
	INT		0x10
	CMP		AX, 0x004f		; 如果有VBE，AX就会变成0x004f
	JNE		scrn320			; 没有VBE，只能用基础分辨率了
; 检查VBE的版本
	MOV		AX, [ES:DI+4]
	CMP		AX, 0x0200		; 0x0200: VBE 2.0
	JB		scrn320			; 版本未够，也只能用基础分辨率
; 取得画面模式信息
	MOV 	CX, VBEMODE
	MOV		AX, 0x4f01
	INT		0x10			; 画面信息被写入[ES:DI]开始的256字节中（这次会覆盖VBE版本信息）
	CMP		AX, 0x004f		; 如果不相等，说明所选画面模式不能使用
	JNE		scrn320			; 显卡不支持，也只能用基础分辨率
; 确认画面模式信息
	; 画面模式信息中重要的信息：需确认⚪
	; WORD	[ES:DI+0x00]	模式属性。（要能加上0x4000的话bit7得是1）⚪
	; WORD	[ES:DI+0x12]	分辨率x
	; WORD	[ES:DI+0x14]	分辨率y
	; BYTE	[ES:DI+0x19]	颜色数（得是8）⚪
	; BYTE	[ES:DI+0x1b]	颜色的指定方法（得是4，即调色板模式）⚪
	; DWORD	[ES:DI+0x28]	VRAM地址
	CMP		BYTE [ES:DI+0x19], 8
	JNE		scrn320
	CMP		BYTE [ES:DI+0x1b], 4
	JNE		scrn320
	MOV		AX, [ES:DI+0x00]
	AND 	AX, 0x0080
	JZ		scrn320
; 能留下来的都是精品，进行画面模式的切换，顺便保存信息
	; 设置VESA BIOS extension(VBE)画面：
	; 1. AX = 0x4f02
	; 2. BX = 0x4000 + 画面模式。
	; 3. INT 0x10
	MOV		BX, VBEMODE+0x4000
	MOV 	AX, 0x4f02
	INT		0x10
	MOV		BYTE [VMODE], 8
	MOV		AX, [ES:DI+0x12]
	MOV		[SCRNX], AX
	MOV		AX, [ES:DI+0x14]
	MOV		[SCRNY], AX
	MOV		EAX, [ES:DI+0x28]
	MOV		[VRAM], EAX
	JMP		keystatus		; 跳过 scrn320 执行后面的 keystatus
; 被过程淘汰的在这里设置画面模式并保存信息
scrn320:
	; 设置IBM BIOS画面模式
    ; 1. MOV     AL, 0x13        ; VGA显卡，320x200，8位调色板
    ; 2. MOV     AH, 0x00        ; 设置显卡模式的标头
	; 3. INT	 0x10
	MOV     AL, 0x13
	MOV     AH, 0x00
    INT     0x10
    MOV     BYTE [VMODE], 8 
    MOV     WORD [SCRNX], 320
    MOV     WORD [SCRNY], 200
    MOV     DWORD [VRAM], 0x000a0000	; VRAM 0xa0000~0xaffff

; 通过BIOS取得键盘上各LED灯状态
keystatus:
    MOV     AH, 0x02
    INT     0x16            ; 键盘BIOS
    MOV     [LEDS], AL		; 4，5，6位依次是ScrollLock, NumLock, CapsLock

; PIC关闭一切中断
	; 根据AT兼容机的规格，如果要初始化PIC，
	; 必须在CLI之前进行，否则有时会挂起。
	; 随后进行PIC的初始化。
	MOV		AL, 0xff
	OUT		0x21, AL		; PIC0_IMR 0xff
	; 如果连续执行OUT指令，有些机种会无法正常运行
	NOP						; 所以使用 NOP 指令什么也不干，休息一个时钟
	OUT		0xa1, AL		; PIC1_IMR 0xff
	CLI						; 禁止CPU级别的中断

; 为了让CPU能够访问1MB以上的内存空间，设定A20GATE
	; 键盘控制电路附属端口 PORT_KEYDAT 连接主板上的很多地方，
	; 通过命令 PORT_KEYCMD 可以让它实现各种控制功能
	CALL	waitkbdout		; wait_KBC_sendready
	MOV		AL, 0xd1
	OUT		0x64, AL		; PORT_KEYCMD, KEYCMD_WRITE_OUTPORT
	CALL	waitkbdout
	MOV		AL, 0xdf	
	OUT		0x60, AL		; PORT_KEYDAT, KBC_OUTPORT_A20G_ENABLE
	CALL	waitkbdout		; 等待A20GATE处理切实完成

; 准备切换到32位保护模式
	LGDT	[GDTR0]			; 设置临时GDT
	MOV		EAX, CR0		; CR0: control register 0 只有系统能操作
	AND		EAX, 0x7fffffff	; 设置bit31为0（禁用分页）
	OR		EAX, 0x00000001	; 设置bit0为1（为了切换到保护模式）
	MOV		CR0, EAX		; 借用 EAX 中继完成对 CR0 的运算
	; 通过代入CR0切换到保护模式时要马上执行JMP指令，
	; 这是因为变为保护模式后，机器语言解释要发生变化（内部微指令），
	; CPU为了加快指令执行速度采用流水线(pipeline)机制，
	; 前一条指令还在执行时就开始解释接下来的指令。
	; 因为模式改变，需要重新解释，所以要用JMP指令。
	JMP		pipelineflush	
pipelineflush:
	; 进入保护模式后除了CS之外的所有段寄存器
	; 从0x0000变为0x0008（相当于gdt+1的段）
	MOV		AX, 1*8			; 可读写的段 32bit
	MOV		DS, AX
	MOV		ES, AX
	MOV		FS, AX
	MOV		GS, AX
	MOV		SS, AX

; bootpack传递
	MOV		ESI, bootpack	; 源
	MOV		EDI, BOTPAK		; 目标
	MOV		ECX, 512*1024/4	; 数据大小（单位DWORD）
	CALL	memcpy

; 磁盘数据最终转送到它本来的位置去

; 首先从启动扇区开始
	MOV		ESI, 0x7c00		; 源
	MOV		EDI, DSKCAC		; 目标
	MOV		ECX, 512/4
	CALL	memcpy

; 剩余的全部
	MOV		ESI, DSKCAC0+512; 源
	MOV		EDI, DSKCAC+512	; 目标
	MOV		ECX, 0
	MOV		CL, BYTE [CYLS]	; ECX 是读入柱面数
	IMUL	ECX, 512*18*2/4	; 整数乘扇区大小(512字节)*18柱面*2磁头/4(DWORD)
	SUB		ECX, 512/4		; 减去启动区偏移量
	CALL	memcpy

; 必须由asmhead来完成的工作，至此全部完毕
; 以后就交由bootpack来完成

; bootpack启动

	MOV		EBX, BOTPAK
	MOV		ECX, [EBX+16]
	ADD		ECX, 3			; ECX += 3;
	SHR		ECX, 2			; 右移两位
	JZ		skip			; (jump if zero)没有要传输的东西时
	MOV		ESI, [EBX+20]	; 源
	ADD		ESI, EBX
	MOV		EDI, [EBX+12]	; 目标
	CALL	memcpy
skip:
	MOV		ESP, [EBX+12]	; 堆栈的初始化
	JMP		DWORD 2*8:0x0000001b	; JMP FAR 到第二段0x1b位置（bootpack）

waitkbdout:
	IN		AL, 0x64
	AND		AL, 0x02
	IN		AL, 0x60		; 空读（为了清空数据接收缓冲区中的垃圾数据）
	JNZ		waitkbdout		; AND的结果如果不是0，就跳到waitkbdout
	RET

memcpy:
	MOV		EAX, [ESI]
	ADD		ESI, 4
	MOV		[EDI], EAX
	ADD		EDI, 4
	DEC		ECX				; ECX - 1
	JNZ		memcpy			; 运算结果不为0跳转到memcpy
	RET
; memcpy地址前缀大小

	ALIGNB	16				; 将此处地址对齐到16的整数倍
GDT0:
	RESB	8				; 初始值
	DW		0xffff,0x0000,0x9200,0x00cf	; 可以读写的段（segment）32bit
	DW		0xffff,0x0000,0x9a28,0x0047	; 可执行的文件的32bit寄存器（bootpack用）
	DW		0
GDTR0:
	DW		8*3-1
	DD		GDT0

	ALIGNB	16
bootpack:
