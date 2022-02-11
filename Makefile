#SHELL := powershell.exe
#.SHELLFLAGS := -Command
TOOLPATH 	:= ./z_tools/
INCPATH  	:= $(TOOLPATH)haribote/

MAKE 		:= make
NASM		:= $(TOOLPATH)nask.exe
CC1 		:= $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GAS2NASK	:= $(TOOLPATH)gas2nask.exe -a
OBJ2BIM		:= $(TOOLPATH)obj2bim.exe
MAKEFONT	:= $(TOOLPATH)makefont.exe
BIN2OBJ		+= $(TOOLPATH)bin2obj.exe
BIM2HRB		:= $(TOOLPATH)bim2hrb.exe
RULEFILE    := $(TOOLPATH)jos.rul
EDIMG    	:= $(TOOLPATH)edimg.exe
IMGTOL      := $(TOOLPATH)imgtol.com
# NASM		:= nasm
IMG			:= jos.img

.PHONY: 
	default clean img run

default:
	@$(MAKE) run

# 生成文件

ipl.bin: ipl.nas Makefile
	@$(NASM) ipl.nas ipl.bin ipl.lst

asmhead.bin: asmhead.nas Makefile
	@$(NASM) asmhead.nas asmhead.bin asmhead.lst

bootpack.gas: bootpack.c Makefile
	@$(CC1) -o bootpack.gas bootpack.c

bootpack.nas: bootpack.gas Makefile
	@$(GAS2NASK) bootpack.gas bootpack.nas

bootpack.obj: bootpack.nas Makefile
	@$(NASM) bootpack.nas bootpack.obj bootpack.lst

naskfunc.obj: naskfunc.nas Makefile
	@$(NASM) naskfunc.nas naskfunc.obj naskfunc.lst

font.bin: font.txt Makefile
	@$(MAKEFONT) font.txt font.bin

font.obj: font.bin Makefile
	@$(BIN2OBJ) font.bin font.obj _font

bootpack.bim: bootpack.obj naskfunc.obj font.obj Makefile
	@$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k map:bootpack.map \
		bootpack.obj naskfunc.obj font.obj

bootpack.hrb: bootpack.bim Makefile
	@$(BIM2HRB) bootpack.bim bootpack.hrb 0

jos.sys: asmhead.bin bootpack.hrb Makefile
	@copy /B asmhead.bin+bootpack.hrb jos.sys > nul

$(IMG): ipl.bin jos.sys Makefile
	@$(EDIMG) imgin:$(TOOLPATH)fdimg0at.tek \
		wbinimg src:ipl.bin len:512 from:0 to:0 \
		copy from:jos.sys to:@: \
		imgout:$(IMG)

# 指令

clean:
	-@del *.bin *.lst *.gas *.map *.bim *.hrb *.obj *.vfd *.img *.sys
	-@del bootpack.nas

img:
	@make $(IMG)

run: $(IMG) Makefile
	@set SDL_VIDEODRIVER=windib
	@set QEMU_AUDIO_DRV=none
	@set QEMU_AUDIO_LOG_TO_MONITOR=0
	@.\z_tools\qemu\qemu.exe -fda $(IMG) -L .\z_tools\qemu -m 32 -localtime -std-vga