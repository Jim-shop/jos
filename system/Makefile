TOOLPATH 	:= ../build_tools/

NASK		:= $(TOOLPATH)nask.exe
INCPATH  	:= $(TOOLPATH)include
CC1 		:= $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GAS2NASK	:= $(TOOLPATH)gas2nask.exe -a
OBJ2BIM		:= $(TOOLPATH)obj2bim.exe
MAKEFONT	:= $(TOOLPATH)makefont.exe
BIN2OBJ		:= $(TOOLPATH)bin2obj.exe
BIM2HRB		:= $(TOOLPATH)bim2hrb.exe
RULEFILE    := $(TOOLPATH)jos.rul

###

.PHONY:
	default clean

default: ipl.bin jos.sys

clean:
	-@del *.gas *.obj *.je *.bin *.sys *.bim


### 启动区编译

ipl.bin: ipl.nas Makefile
	@$(NASK) ipl.nas ipl.bin


# 所有.c .nas文件编译到.obj
%.gas: %.c Makefile
	@$(CC1) -o $*.gas $*.c
%.nas: %.gas Makefile
	@$(GAS2NASK) $*.gas $*.nas
%.obj: %.nas Makefile
	@$(NASK) $*.nas $*.obj

# 字库编译到.obj
font.bin: font.txt Makefile
	@$(MAKEFONT) font.txt font.bin
font.obj: font.bin Makefile
	@$(BIN2OBJ) font.bin font.obj _font

# 所需.obj编译到bootpack.je
OBJS_BOOTPACK := font.obj bootpack.obj dsctbl.obj graphic.obj int.obj fifo.obj keyboard.obj mouse.obj memory.obj sheet.obj timer.obj mtask.obj window.obj file.obj console.obj naskfunc.obj 
bootpack.bim: $(OBJS_BOOTPACK) Makefile
	@$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k $(OBJS_BOOTPACK)
bootpack.je: bootpack.bim Makefile
	@$(BIM2HRB) bootpack.bim bootpack.je 0

# 汇编部分编译到.bin
asmhead.bin: asmhead.nas Makefile
	@$(NASK) asmhead.nas asmhead.bin

# 融合bootpack.je asmhead.bin生成.sys系统文件
jos.sys: asmhead.bin bootpack.je Makefile
	@copy /B asmhead.bin+bootpack.je jos.sys > nul

