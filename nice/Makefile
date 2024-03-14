
BINARIES=nice.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:nice.pdb

BIN_OBJS=\
	 nice.obj         \

MOD_OBJS=\
	 mnice.obj     \

compile: $(BIN_OBJS) builtins.lib

nice.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mnice.obj: nice.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ nice.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

