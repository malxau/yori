
BINARIES=ymove.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ymove.pdb

BIN_OBJS=\
	 move.obj         \

MOD_OBJS=\
	 mod_move.obj     \

compile: $(BIN_OBJS) builtins.lib

ymove.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_move.obj: move.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ move.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
