
BINARIES=hilite.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:hilite.pdb

BIN_OBJS=\
	 hilite.obj         \

MOD_OBJS=\
	 mhilite.obj     \

compile: $(BIN_OBJS) builtins.lib

hilite.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mhilite.obj: hilite.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ hilite.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


