
BINARIES=ymem.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ymem.pdb

BIN_OBJS=\
	 mem.obj         \

MOD_OBJS=\
	 mmem.obj     \

compile: $(BIN_OBJS) builtins.lib

ymem.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mmem.obj: mem.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ mem.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
