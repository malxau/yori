
BINARIES=ycharmap.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ycharmap.pdb

BIN_OBJS=\
	 charmap.obj         \

MOD_OBJS=\
	 mcharmap.obj     \

compile: $(BIN_OBJS) builtins.lib

ycharmap.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mcharmap.obj: charmap.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ charmap.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

