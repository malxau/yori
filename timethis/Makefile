
BINARIES=timethis.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:timethis.pdb

BIN_OBJS=\
	 timethis.obj         \

MOD_OBJS=\
	 mtime.obj     \

compile: $(BIN_OBJS) builtins.lib

timethis.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mtime.obj: timethis.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ timethis.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

