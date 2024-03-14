
BINARIES=yexpr.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:yexpr.pdb

BIN_OBJS=\
	 expr.obj         \

MOD_OBJS=\
	 mexpr.obj     \

compile: $(BIN_OBJS) builtins.lib

yexpr.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mexpr.obj: expr.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ expr.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


