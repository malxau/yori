
BINARIES=yextents.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:yextents.pdb

BIN_OBJS=\
	 extents.obj         \

MOD_OBJS=\
	 mextents.obj     \

compile: $(BIN_OBJS) builtins.lib

yextents.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mextents.obj: extents.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ extents.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
