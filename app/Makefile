APP			:= $(subst .c,.je,$(wildcard *.c))
APILIB 		:= ../apilib/apilib.lib

TOOLPATH 	:= ../build_tools/

NASK		:= $(TOOLPATH)nask.exe
INCPATH  	:= $(TOOLPATH)include
CC1 		:= $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GAS2NASK	:= $(TOOLPATH)gas2nask.exe -a
OBJ2BIM		:= $(TOOLPATH)obj2bim.exe
BIM2HRB		:= $(TOOLPATH)bim2hrb.exe
RULEFILE    := $(TOOLPATH)jos.rul

###

.PHONY: 
	default clean

default: $(APP)

clean:
	@cd ../apilib && make clean
	-@del *.bin *.lst *.gas *.nas *.bim *.obj *.je


###

$(APILIB):
	@cd ../apilib && make

%.gas: %.c Makefile
	@$(CC1) -o $*.gas $*.c

%.nas: %.gas Makefile
	@$(GAS2NASK) $*.gas $*.nas

%.obj: %.nas Makefile
	@$(NASK) $*.nas $*.obj

%.bim: %.obj $(APILIB) Makefile
	@$(OBJ2BIM) @$(RULEFILE) out:$*.bim stack:1k $*.obj $(APILIB)

%.je: %.bim Makefile
	@$(BIM2HRB) $*.bim $*.je 100k
