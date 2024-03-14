
BINARIES=repl.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:repl.pdb

BIN_OBJS=\
	 repl.obj         \

MOD_OBJS=\
	 mrepl.obj     \

compile: $(BIN_OBJS) builtins.lib

repl.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mrepl.obj: repl.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ repl.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
