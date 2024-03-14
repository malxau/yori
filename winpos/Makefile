
BINARIES=winpos.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:winpos.pdb

BIN_OBJS=\
	 winpos.obj         \

MOD_OBJS=\
	 mwinpos.obj     \

compile: $(BIN_OBJS) builtins.lib

winpos.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mwinpos.obj: winpos.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ winpos.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
