
BINARIES=procinfo.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:procinfo.pdb

BIN_OBJS=\
	 procinfo.obj         \

MOD_OBJS=\
	 mproc.obj     \

compile: $(BIN_OBJS) builtins.lib

procinfo.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mproc.obj: procinfo.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ procinfo.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

