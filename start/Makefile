
BINARIES=ystart.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ystart.pdb

BIN_OBJS=\
	 start.obj         \

MOD_OBJS=\
	 mstart.obj     \

compile: $(BIN_OBJS) builtins.lib

ystart.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mstart.obj: start.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ start.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

