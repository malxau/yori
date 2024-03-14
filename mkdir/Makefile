
BINARIES=ymkdir.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ymkdir.pdb

BIN_OBJS=\
	 mkdir.obj        \

MOD_OBJS=\
	 mmkdir.obj    \

compile: $(BIN_OBJS) builtins.lib

ymkdir.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mmkdir.obj: mkdir.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ mkdir.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
