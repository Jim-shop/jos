IMG			:= jos.vfd
SYS			:= ./system/jos.sys
IPL 		:= ./system/ipl.bin
APP			:= $(wildcard ./app/*.je)

TOOLPATH 	:= ./build_tools/

EDIMG    	:= $(TOOLPATH)edimg.exe
RULEFILE    := $(TOOLPATH)jos.rul
IMGTOL      := $(TOOLPATH)imgtol.com
QEMUPATH	:= $(TOOLPATH)qemu/
QEMU		:= $(QEMUPATH)qemu.exe

###

.PHONY: 
	default clean img run system app

default: run

clean: 
	@cd system && make clean
	@cd app && make clean
	-@del *.img *.vfd

img: 
	@cd system && make
	@cd app && make
	@make $(IMG)

run: img
	@set SDL_VIDEODRIVER=windib
	@set QEMU_AUDIO_DRV=none
	@set QEMU_AUDIO_LOG_TO_MONITOR=0
	@$(QEMU) -fda $(IMG) -L $(QEMUPATH) -m 32 -localtime -std-vga


### 

$(IMG): Makefile 
	@$(EDIMG) imgin:$(TOOLPATH)fdimg0at.tek \
		wbinimg src:$(IPL) len:512 from:0 to:0 \
		copy from:$(SYS) to:@: \
		$(patsubst %.je,copy from:%.je to:@:,$(APP)) \
		imgout:$(IMG)
