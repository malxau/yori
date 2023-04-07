
BINARIES=yget.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:yget.pdb

BIN_OBJS=\
	 get.obj         \

MOD_OBJS=\
	 mod_get.obj     \

compile: $(BIN_OBJS) builtins.lib

yget.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_get.obj: get.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ get.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@



