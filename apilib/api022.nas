; 22. 关闭文件（EAX=文件句柄）

[FORMAT "WCOFF"]                ; 生成对象文件的格式
[INSTRSET "i486p"]              ; 486兼容指令集
[BITS 32]                       ; 生成32位模式机器语言
[FILE "api022.nas"]             ; 源文件名

    GLOBAL	_api_fclose

[SECTION .text]

_api_fclose:        ; void api_fclose(int fhandle);
    MOV		EDX, 22
    MOV		EAX, [ESP+4]		
    INT		0x40
    RET
