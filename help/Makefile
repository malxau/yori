
BINARIES=yhelp.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:yhelp.pdb

BIN_OBJS=\
	 help.obj         \

MOD_OBJS=\
	 mod_help.obj     \

compile: $(BIN_OBJS) builtins.lib

yhelp.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_help.obj: help.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ help.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

