
BINARIES=yerr.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:yerr.pdb

BIN_OBJS=\
	 err.obj         \

MOD_OBJS=\
	 merr.obj     \

compile: $(BIN_OBJS) builtins.lib

yerr.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

merr.obj: err.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ err.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
