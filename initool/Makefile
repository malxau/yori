
BINARIES=initool.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:initool.pdb

BIN_OBJS=\
	 initool.obj         \

MOD_OBJS=\
	 mod_initool.obj     \

compile: $(BIN_OBJS) builtins.lib

initool.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_initool.obj: initool.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ initool.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

