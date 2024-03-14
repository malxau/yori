
BINARIES=yecho.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:yecho.pdb

BIN_OBJS=\
	 echo.obj         \

MOD_OBJS=\
	 mecho.obj     \

compile: $(BIN_OBJS) builtins.lib

yecho.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mecho.obj: echo.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ echo.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


