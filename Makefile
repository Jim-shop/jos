#SHELL := powershell.exe
#.SHELLFLAGS := -Command
MAKE 		= make
NASM		= ./z_tools/nask.exe
#NASM		= nasm

.PHONY: default asm vfd clean run run_hyperv

default:
	@$(MAKE) run

# 生成文件

ipl.bin: ipl.nas Makefile
	@$(NASM) ipl.nas ipl.bin

ipl.vfd: ipl.bin Makefile
	@./z_tools/edimg.exe imgin:./z_tools/fdimg0at.tek wbinimg src:ipl.bin len:512 from:0 to:0 imgout:ipl.vfd

# 指令

asm:
	@$(MAKE) ipl.bin

vfd:
	@$(MAKE) ipl.vfd

clean:
	@del *.bin *.vfd

run: ipl.vfd Makefile
	@set SDL_VIDEODRIVER=windib
	@set QEMU_AUDIO_DRV=none
	@set QEMU_AUDIO_LOG_TO_MONITOR=0
	@.\z_tools\qemu\qemu.exe -L .\z_tools\qemu -m 32 -localtime -std-vga -fda ipl.vfd

run_hyperv: 
	@echo 功能尚未完成……
