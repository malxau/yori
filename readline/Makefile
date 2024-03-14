
BINARIES=readline.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:readline.pdb

BIN_OBJS=\
	 readline.obj         \

MOD_OBJS=\
	 mreadln.obj          \

compile: $(BIN_OBJS) builtins.lib

readline.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mreadln.obj: readline.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ readline.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
