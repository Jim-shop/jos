#SHELL := powershell.exe
#.SHELLFLAGS := -Command

.PHONY: default asm vfd clean run_qemu run_hyperv

default:
	@make run_qemu

ipl.bin: ipl.nas Makefile
	@./z_tools/nask.exe ipl.nas ipl.bin

ipl.vfd: ipl.bin Makefile
	@./z_tools/edimg.exe imgin:./z_tools/fdimg0at.tek wbinimg src:ipl.bin len:512 from:0 to:0 imgout:ipl.vfd

asm:
	@make ipl.bin

vfd:
	@make ipl.vfd

clean:
	@del *.bin *.vfd

run_qemu: ipl.vfd Makefile
	@set SDL_VIDEODRIVER=windib
	@set QEMU_AUDIO_DRV=none
	@set QEMU_AUDIO_LOG_TO_MONITOR=0
	@.\z_tools\qemu\qemu.exe -L .\z_tools\qemu -m 32 -localtime -std-vga -fda ipl.vfd

run_hyperv: 
	@echo ������δ��ɡ���
