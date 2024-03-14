
BINARIES=ypause.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ypause.pdb

BIN_OBJS=\
	 pause.obj         \

MOD_OBJS=\
	 mpause.obj     \

compile: $(BIN_OBJS) builtins.lib

ypause.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mpause.obj: pause.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ pause.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


