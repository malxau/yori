
BINARIES=ysponge.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ysponge.pdb

BIN_OBJS=\
	 sponge.obj         \

MOD_OBJS=\
	 mod_sponge.obj     \

compile: $(BIN_OBJS) builtins.lib

ysponge.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_sponge.obj: sponge.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ sponge.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

