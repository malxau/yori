
BINARIES=yobjdir.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:yobjdir.pdb

BIN_OBJS=\
	 objdir.obj         \

MOD_OBJS=\
	 mod_objdir.obj     \

compile: $(BIN_OBJS) builtins.lib

yobjdir.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_objdir.obj: objdir.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ objdir.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


