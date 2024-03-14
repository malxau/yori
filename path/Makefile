
BINARIES=ypath.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ypath.pdb

BIN_OBJS=\
	 path.obj        \

MOD_OBJS=\
	 mpath.obj    \

compile: $(BIN_OBJS) builtins.lib

ypath.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mpath.obj: path.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ path.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

