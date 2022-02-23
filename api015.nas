; 15. 获取键盘/定时器输入（EAX={0:不阻塞，没有输入时返回-1; 1:阻塞}）->EAX=输入的字符编码

[FORMAT "WCOFF"]                ; 生成对象文件的格式
[INSTRSET "i486p"]              ; 486兼容指令集
[BITS 32]                       ; 生成32位模式机器语言
[FILE "api015.nas"]             ; 源文件名

    GLOBAL  _api_getkey

[SECTION .text]

_api_getkey:		; int api_getkey(int mode);
    MOV		EDX, 15
    MOV		EAX, [ESP+4]
    INT		0x40
    RET
