
BINARIES=yintcmp.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:yintcmp.pdb

BIN_OBJS=\
	 intcmp.obj        \

MOD_OBJS=\
	 mod_intcmp.obj    \

compile: $(BIN_OBJS) builtins.lib

yintcmp.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_intcmp.obj: intcmp.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ intcmp.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
