
BINARIES=ycompact.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ycompact.pdb

BIN_OBJS=\
	 compact.obj         \

MOD_OBJS=\
	 mcompact.obj     \

compile: $(BIN_OBJS) builtins.lib

ycompact.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mcompact.obj: compact.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ compact.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

