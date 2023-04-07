
BINARIES=ytype.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ytype.pdb

BIN_OBJS=\
	 type.obj         \

MOD_OBJS=\
	 mod_type.obj     \

compile: $(BIN_OBJS) builtins.lib

ytype.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_type.obj: type.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ type.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
