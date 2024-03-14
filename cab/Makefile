
BINARIES=cab.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:cab.pdb

BIN_OBJS=\
	 cab.obj         \

MOD_OBJS=\
	 mcab.obj     \

compile: $(BIN_OBJS) builtins.lib

cab.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) $(FDILIB) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mcab.obj: cab.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ cab.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

