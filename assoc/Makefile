
BINARIES=yassoc.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:yassoc.pdb

BIN_OBJS=\
	 assoc.obj         \

MOD_OBJS=\
	 mod_assoc.obj     \

compile: $(BIN_OBJS) builtins.lib

yassoc.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_assoc.obj: assoc.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ assoc.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
