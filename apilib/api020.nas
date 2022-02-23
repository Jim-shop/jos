; 20. 蜂鸣器发声（EAX=声音频率mHz,0表示停止发声）

[FORMAT "WCOFF"]                ; 生成对象文件的格式
[INSTRSET "i486p"]              ; 486兼容指令集
[BITS 32]                       ; 生成32位模式机器语言
[FILE "api020.nas"]             ; 源文件名

    GLOBAL	_api_beep

[SECTION .text]

_api_beep:			; void api_beep(int tone);
    MOV		EDX, 20
    MOV		EAX, [ESP+4]		
    INT		0x40
    RET
