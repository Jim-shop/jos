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
	@$(NASM) ipl.nas ipl.bin ipl.lst


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
	@$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k map:bootpack.map $(OBJS_BOOTPACK)
bootpack.je: bootpack.bim Makefile
	@$(BIM2HRB) bootpack.bim bootpack.je 0

# ��ಿ�ֱ��뵽.bin
asmhead.bin: asmhead.nas Makefile
	@$(NASM) asmhead.nas asmhead.bin

# �ں�bootpack.je asmhead.bin����.sysϵͳ�ļ�
jos.sys: asmhead.bin bootpack.je Makefile
	@copy /B asmhead.bin+bootpack.je jos.sys > nul


### Ӧ��

%.bim: %.obj a_nask.obj Makefile
	@$(OBJ2BIM) @$(RULEFILE) out:$*.bim stack:1k $*.obj a_nask.obj
%.je: %.bim Makefile
	@$(BIM2HRB) $*.bim $*.je 0

hello.je: hello.nas Makefile
	@$(NASM) hello.nas hello.je
hello2.je: hello2.nas Makefile
	@$(NASM) hello2.nas hello2.je
crack2.je: crack2.nas Makefile
	@$(NASM) crack2.nas crack2.je
crack3.je: crack3.nas Makefile
	@$(NASM) crack3.nas crack3.je	
crack4.je: crack4.nas Makefile
	@$(NASM) crack4.nas crack4.je
crack5.je: crack5.nas Makefile
	@$(NASM) crack5.nas crack5.je
hello5.bim: hello5.obj Makefile
	@$(OBJ2BIM) @$(RULEFILE) out:hello5.bim stack:1k map:hello5.map hello5.obj


### �������

JE := hello.je hello2.je a.je hello3.je crack1.je crack2.je crack3.je crack4.je crack5.je bug1.je bug2.je bug3.je hello4.je hello5.je winhelo.je winhelo2.je
$(IMG): ipl.bin jos.sys Makefile $(JE)
	@$(EDIMG) imgin:$(TOOLPATH)fdimg0at.tek \
		wbinimg src:ipl.bin len:512 from:0 to:0 \
		copy from:jos.sys to:@: \
		$(patsubst %.je,copy from:%.je to:@:,$(JE)) \
		imgout:$(IMG)

####### ָ��

clean:
	-@del *.bin *.lst *.gas *.map *.bim *.hrb *.obj

clean_all: clean
	-@del *.img *.vfd *.sys *.je

img: $(IMG)

run: $(IMG) Makefile
	@set SDL_VIDEODRIVER=windib
	@set QEMU_AUDIO_DRV=none
	@set QEMU_AUDIO_LOG_TO_MONITOR=0
	@.\z_tools\qemu\qemu.exe -fda $(IMG) -L .\z_tools\qemu -m 32 -localtime -std-vga