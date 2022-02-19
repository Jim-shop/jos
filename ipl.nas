; jos-ipl

; 声明汇编常数
CYLS EQU 10                 ; 读取的柱面数量

    ORG     0x7c00          ; 知会汇编程序 程序被装载到的内存地址（0x7c00是BIOS跳转地址）

; 以下的记述用于标准FAT12格式的软盘

    JMP     entry
    DB      0x90
    DB		"jos     "		; OEM引导扇区名称（8 字节）
    DW		512				; 1个扇区大小（必须为 512）
    DB		1				; 簇大小（必须是一个扇区）
    DW		1				; FAT 预留扇区数（通常从第一个扇区开始）
    DB		2				; FAT 表数量（必须为 2）
    DW		224				; 根目录的大小（通常为 224 个条目）
    DW		2880			; 磁盘扇区总数（软盘2880），若为0则代表超过65535个扇区，需要使用第22行记录
    DB		0xf0			; 驱动器种类（1.44M软盘为0xf0）
    DW		9				; 每个 FAT 长度（必须为 9 个扇区）
    DW		18				; 一个磁道上有多少个扇区（软盘18个）
    DW		2				; 磁头数（软盘2个）
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
    MOV     AX, 0           ; 用以初始化段寄存器
    MOV     DS, AX          ; 数据段寄存器（默认）
    MOV     SS, AX          ; 栈段寄存器
    MOV     SP, 0x7c00      ; 栈指针寄存器

; 读取硬盘

    MOV     AX, 0x0820      ; 读取到0x8200处
    MOV     ES, AX          ; 附加段寄存器，用于稍后指定缓冲区
    ; 柱面0磁头0扇区1是启动区，已自动装载
    MOV     CH, 0           ; 柱面0（0~79）
    MOV     DH, 0           ; 磁头0（0~1）
    MOV     CL, 2           ; 扇区2（1~18）
readloop:
    MOV     SI, 0           ; 记录失败次数
retry:
    MOV     AH, 0x02        ; 0x02读盘 0x03写 0x04校验 0x0c寻道
    MOV     AL, 1           ; 处理1个扇区
    MOV     BX, 0           ; 缓冲区内存地址 ES:BX
    MOV     DL, 0x00        ; 驱动器号（A盘）
    INT     0x13            ; 调用磁盘BIOS
    JNC     next            ; 没出错的话
    INC     SI              ; 失败计数+1
    CMP     SI, 5           ; 判断失败次数是否达到5
    JAE     error           ; 大于等于的话
    MOV     AH, 0x00        ; 0x00复位
    MOV     DL, 0x00        ; 驱动器号（A盘）
    INT     0x13            ; 调用磁盘BIOS
    JMP     retry
next:
    MOV     AX, ES          ; ES没法直接做加法
    ADD     AX, 0x0020      ; AX加0x0020，输回ES相当于加0x200
    MOV     ES, AX          ; 缓冲区地址后移了512字节
    ; 扇区范围 1~18
    INC     CL              ; 扇区+1（1扇区512字节）
    CMP     CL, 18          ; 判断扇区是否达到18
    JBE     readloop        ; 小于等于的话
    ; 磁头范围 0~1 （正面0，反面1）
    MOV     CL, 1           ; 读完一个磁头，扇区复位
    INC     DH              ; 磁头加一
    CMP     DH, 2
    JB      readloop
    ; 柱面范围 0~79
    MOV     DH, 0           ; 读完一个柱面，磁头复位
    INC     CH              ; CH是柱面
    CMP     CH, CYLS        ; CYLS 是一个常数
    JB      readloop

; 读取完毕
    MOV     [0x0ff0], CH    ; 将CYLS值写入0x0ff0以供后续使用
    ; jos.sys是软盘第一个文件，文件内容储存在软盘0x4200处
    ; 软盘0x200后的内容被读取到内存0x8200处
    ; 所以jos.sys内存地址就是 0x8000 + 0x4200 = 0xc200
    JMP     0xc200          ; 跳转jos.sys执行

error: 
    MOV     SI, msg         ; 字串数据内存位置
putloop:
    MOV     AL,[SI]         ; 读取SI位置字节
    INC     SI              ; 给SI加1
    CMP     AL, 0           ; 判断字节是否为0
    JE      fin             ; 是，代表数据结束，跳转
    MOV     AH, 0x0e        ; 显示文字命令固定标头
    MOV     BX, 15          ; 指定字符颜色
    INT     0x10            ; 调用显卡BIOS
    JMP     putloop
msg:
    DB      0x0a, 0x0a      ; 换行两次
    DB      "load error"
    DB      0x0a            ; 换行
    DB      0               ; 提供结束数据读取标志
fin:
    HLT                     ; 让CPU停止，等待指令
    JMP     fin             ; 无限循环
    
; 启动区内其他内容

    ; 使用ORG指令后$表示内存地址（ORG地址+文件当前位置）
    RESB    0x7dfe-$        ; 用0填充到(0x7c00+0x1fe)
    DB      0x55, 0xaa      ; 启动区结束标志