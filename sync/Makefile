
BINARIES=sync.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:sync.pdb

BIN_OBJS=\
	 sync.obj         \

MOD_OBJS=\
	 msync.obj     \

compile: $(BIN_OBJS) builtins.lib

sync.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

msync.obj: sync.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ sync.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
