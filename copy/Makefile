
BINARIES=ycopy.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ycopy.pdb

BIN_OBJS=\
	 copy.obj         \

MOD_OBJS=\
	 mod_copy.obj     \

compile: $(BIN_OBJS) builtins.lib

ycopy.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_copy.obj: copy.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ copy.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


