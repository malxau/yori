
BINARIES=yvol.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:yvol.pdb

BIN_OBJS=\
	 vol.obj        \

MOD_OBJS=\
	 mod_vol.obj    \

compile: $(BIN_OBJS) builtins.lib

yvol.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_vol.obj: vol.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ vol.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
