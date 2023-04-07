
BINARIES=tee.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:tee.pdb

BIN_OBJS=\
	 tee.obj         \

MOD_OBJS=\
	 mod_tee.obj     \

compile: $(BIN_OBJS) builtins.lib

tee.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_tee.obj: tee.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ tee.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
