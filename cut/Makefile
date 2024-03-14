
BINARIES=ycut.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ycut.pdb

BIN_OBJS=\
	 cut.obj         \

MOD_OBJS=\
	 mcut.obj     \

compile: $(BIN_OBJS) builtins.lib

ycut.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mcut.obj: cut.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ cut.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


