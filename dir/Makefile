
BINARIES=ydir.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ydir.pdb

BIN_OBJS=\
	 dir.obj         \

MOD_OBJS=\
	 mdir.obj     \

compile: $(BIN_OBJS) builtins.lib

ydir.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mdir.obj: dir.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ dir.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


