API 		:= $(subst .nas,.obj,$(wildcard *.nas))

TOOLPATH 	:= ../build_tools/
NASK		:= $(TOOLPATH)nask.exe
GOLIB		:= $(TOOLPATH)golib00.exe

###

.PHONY:
	default clean

default: apilib.lib

clean:
	-@del *.obj *.lib

###

%.obj: %.nas Makefile
	@$(NASK) $*.nas $*.obj

apilib.lib: $(API) Makefile
	@$(GOLIB) $(API) out:apilib.lib
