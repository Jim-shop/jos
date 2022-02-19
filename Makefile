#SHELL := powershell.exe
#.SHELLFLAGS := -Command
TOOLPATH 	:= ./z_tools/
TOOLKIT		:= ./toolkit
INCPATH  	:= ./include

MAKE 		:= make
NASM		:= $(TOOLPATH)nask.exe
CC1 		:= $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GAS2NASK	:= $(TOOLPATH)gas2nask.exe -a
OBJ2BIM		:= $(TOOLPATH)obj2bim.exe
MAKEFONT	:= $(TOOLPATH)makefont.exe
BIN2OBJ		:= $(TOOLPATH)bin2obj.exe
BIM2HRB		:= $(TOOLPATH)bim2hrb.exe
RULEFILE    := $(TOOLKIT)/jos.rul
EDIMG    	:= $(TOOLPATH)edimg.exe
IMGTOL      := $(TOOLPATH)imgtol.com
IMG			:= jos.vfd

.PHONY: 
	default clean img run

default:
	@$(MAKE) run

####### 生成文件

### 启动区编译

ipl.bin: ipl.nas Makefile
	@$(NASM) ipl.nas ipl.bin ipl.lst


### 系统编译

# 所有.c .nas文件编译到.obj
%.gas: %.c Makefile
	@$(CC1) -o $*.gas $*.c
%.nas: %.gas Makefile
	@$(GAS2NASK) $*.gas $*.nas
%.obj: %.nas Makefile
	@$(NASM) $*.nas $*.obj $*.lst

# 字库编译到.obj
font.bin: font.txt Makefile
	@$(MAKEFONT) font.txt font.bin
font.obj: font.bin Makefile
	@$(BIN2OBJ) font.bin font.obj _font

# 所需.obj编译到.hrb
OBJS_BOOTPACK := naskfunc.obj font.obj bootpack.obj dsctbl.obj graphic.obj int.obj fifo.obj keyboard.obj mouse.obj memory.obj sheet.obj timer.obj mtask.obj window.obj file.obj console.obj
bootpack.bim: $(OBJS_BOOTPACK) Makefile
	@$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k map:bootpack.map $(OBJS_BOOTPACK)
bootpack.hrb: bootpack.bim Makefile
	@$(BIM2HRB) bootpack.bim bootpack.hrb 0

# 汇编部分编译到.bin
asmhead.bin: asmhead.nas Makefile
	@$(NASM) asmhead.nas asmhead.bin asmhead.lst

# 融合.hrb .bin生成.sys系统文件
jos.sys: asmhead.bin bootpack.hrb Makefile
	@copy /B asmhead.bin+bootpack.hrb jos.sys > nul

### 打包镜像

$(IMG): ipl.bin jos.sys Makefile
	@$(EDIMG) imgin:$(TOOLPATH)fdimg0at.tek \
		wbinimg src:ipl.bin len:512 from:0 to:0 \
		copy from:jos.sys to:@: \
		copy from:bootpack.h to:@: \
		copy from:bootpack.c to:@: \
		imgout:$(IMG)

####### 指令

clean:
	-@del *.bin *.lst *.gas *.map *.bim *.hrb *.obj *.sys

clean_all: clean
	-@del *.img *.vfd

img: 
	@$(MAKE) $(IMG)

run: $(IMG) Makefile
	@set SDL_VIDEODRIVER=windib
	@set QEMU_AUDIO_DRV=none
	@set QEMU_AUDIO_LOG_TO_MONITOR=0
	@.\z_tools\qemu\qemu.exe -fda $(IMG) -L .\z_tools\qemu -m 32 -localtime -std-vga