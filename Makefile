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
	default clean clean_all img run

default:
	@$(MAKE) run

%: Makefile

####### �����ļ�

### ����������

ipl.bin: ipl.nas Makefile
	@$(NASM) ipl.nas ipl.bin


### ϵͳ����

# ����.c .nas�ļ����뵽.obj
%.gas: %.c Makefile
	@$(CC1) -o $*.gas $*.c
%.nas: %.gas Makefile
	@$(GAS2NASK) $*.gas $*.nas
%.obj: %.nas Makefile
	@$(NASM) $*.nas $*.obj

# �ֿ���뵽.obj
font.bin: font.txt Makefile
	@$(MAKEFONT) font.txt font.bin
font.obj: font.bin Makefile
	@$(BIN2OBJ) font.bin font.obj _font

# ����.obj���뵽bootpack.je
OBJS_BOOTPACK := font.obj bootpack.obj dsctbl.obj graphic.obj int.obj fifo.obj keyboard.obj mouse.obj memory.obj sheet.obj timer.obj mtask.obj window.obj file.obj console.obj naskfunc.obj 
bootpack.bim: $(OBJS_BOOTPACK) Makefile
	@$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k $(OBJS_BOOTPACK)
bootpack.je: bootpack.bim Makefile
	@$(BIM2HRB) bootpack.bim bootpack.je 0

# ��ಿ�ֱ��뵽.bin
asmhead.bin: asmhead.nas Makefile
	@$(NASM) asmhead.nas asmhead.bin

# �ں�bootpack.je asmhead.bin����.sysϵͳ�ļ�
jos.sys: asmhead.bin bootpack.je Makefile
	@copy /B asmhead.bin+bootpack.je jos.sys > nul


### Ӧ��

%.bim: %.obj api.obj Makefile
	@$(OBJ2BIM) @$(RULEFILE) out:$*.bim stack:1k $*.obj api.obj
%.je: %.bim Makefile
	@$(BIM2HRB) $*.bim $*.je 100k


### �������

JE := a.je loop.je winhelo.je winhelo2.je winhelo3.je star1.je stars.je stars2.je lines.je walk.je noodle.je beepdown.je color.je color2.je crack7.je
$(IMG): ipl.bin jos.sys Makefile $(JE)
	@$(EDIMG) imgin:$(TOOLPATH)fdimg0at.tek \
		wbinimg src:ipl.bin len:512 from:0 to:0 \
		copy from:jos.sys to:@: \
		$(patsubst %.je,copy from:%.je to:@:,$(JE)) \
		imgout:$(IMG)


####### ָ��

clean:
	-@del *.bin *.lst *.gas *.map *.bim *.hrb *.obj *.sys *.je

clean_all: clean
	-@del *.img *.vfd

img: $(IMG)

run: $(IMG) Makefile
	@set SDL_VIDEODRIVER=windib
	@set QEMU_AUDIO_DRV=none
	@set QEMU_AUDIO_LOG_TO_MONITOR=0
	@.\z_tools\qemu\qemu.exe -fda $(IMG) -L .\z_tools\qemu -m 32 -localtime -std-vga