
BINARIES=ydbg.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ydbg.pdb

BIN_OBJS=\
	 dbg.obj         \

MOD_OBJS=\
	 mod_dbg.obj     \

compile: $(BIN_OBJS) builtins.lib

ydbg.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_dbg.obj: dbg.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ dbg.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


