
BINARIES=tail.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:tail.pdb

BIN_OBJS=\
	 tail.obj         \

MOD_OBJS=\
	 mod_tail.obj     \

compile: $(BIN_OBJS) builtins.lib

tail.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_tail.obj: tail.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ tail.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
