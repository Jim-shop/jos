; jos-1
; TAB=4

    ORG     0x7c00          ; 指明程序的装载地址

; 以下的记述用于标准FAT12格式的软盘

    JMP     entry
    DB      0x90
    DB		"josIPL  "		; OEM引导扇区名称（8 字节）
    DW		512				; 1个扇区大小（必须为 512）
    DB		1				; 簇大小（必须是一个扇区）
    DW		1				; FAT 预留扇区数（通常从第一个扇区开始）
    DB		2				; FAT 表数量（必须为 2）
    DW		224				; 根目录的大小（通常为 224 个条目）
    DW		2880			; 磁盘扇区总数（软盘2880），若为0则代表超过65535个扇区，需要使用第22行记录
    DB		0xf0			; 驱动器种类（1.44M软盘为0xf0）
    DW		9				; 每个 FAT 长度（必须为 9 个扇区）
    DW		18				; 一个磁道上有多少个扇区（必须是 18）
    DW		2				; 磁头数（必须为 2）
    DD		0				; 隐藏的扇区数
    DD		2880			; 大容量扇区总数（如果16行为0则使用本行记录）
    DB		0               ; 中断0x13的设备号
    DB      0               ; Windows NT 标识符
    DB      0x29		    ; 扩展引导标识
    DD		0xffffffff		; 卷序列号
    DB		"jos        "	; 卷标（11 个字节）
    DB		"FAT12   "		; 文件系统类型（8 字节）
    RESB	18				; 先空出 18 字节

; 程序主体

entry:
    MOV     AX, 0           ; 初始化寄存器
    MOV     SS, AX
    MOV     SP, 0x7c00
    MOV     DS, AX
    MOV     ES, AX
    MOV     BH, 0
    MOV     SI, msg
putloop:
    MOV     AL,[SI]
    ADD     SI, 1           ; 给SI加1
    CMP     AL, 0
    JE      fin
    MOV     AH, 0x0e        ; 显示文字固定标头
    MOV     BX, 15          ; 指定字符颜色
    INT     0x10            ; 调用显卡BIOS
    JMP     putloop
fin:
    HLT                     ; 让CPU停止，等待指令
    JMP     fin             ; 无限循环
msg:
    DB      0x0a, 0x0a      ; 换行两次
    DB      "hello, world"
    DB      0x0a            ; 换行
    DB      0               ; 提供结束数据读取标志

; 启动区内其他内容

    RESB    0x7dfe-$        ; 用0填充到0x7dfe
    DB      0x55, 0xaa      ; 启动区结束标志