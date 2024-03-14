
BINARIES=ymklink.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ymklink.pdb

BIN_OBJS=\
	 mklink.obj         \

MOD_OBJS=\
	 mmklink.obj     \

compile: $(BIN_OBJS) builtins.lib

ymklink.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mmklink.obj: mklink.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ mklink.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
