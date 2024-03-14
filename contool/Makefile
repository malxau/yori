
BINARIES=contool.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:contool.pdb

BIN_OBJS=\
	 contool.obj        \

MOD_OBJS=\
	 mcontool.obj    \

compile: $(BIN_OBJS) builtins.lib

contool.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mcontool.obj: contool.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ contool.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
