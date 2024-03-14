
BINARIES=lines.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:lines.pdb

BIN_OBJS=\
	 lines.obj         \

MOD_OBJS=\
	 mlines.obj     \

compile: $(BIN_OBJS) builtins.lib

lines.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mlines.obj: lines.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ lines.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

