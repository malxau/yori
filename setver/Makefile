
BINARIES=setver.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:setver.pdb

BIN_OBJS=\
	 setver.obj         \

MOD_OBJS=\
	 msetver.obj     \

compile: $(BIN_OBJS) builtins.lib

setver.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

msetver.obj: setver.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ setver.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
