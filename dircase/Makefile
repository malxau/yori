
BINARIES=ydircase.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ydircase.pdb

BIN_OBJS=\
	 dircase.obj         \

MOD_OBJS=\
	 mdircase.obj     \

compile: $(BIN_OBJS) builtins.lib

ydircase.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mdircase.obj: dircase.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ dircase.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

