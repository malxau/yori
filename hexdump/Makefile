
BINARIES=hexdump.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:hexdump.pdb

BIN_OBJS=\
	 hexdump.obj         \

MOD_OBJS=\
	 mhexdump.obj     \

compile: $(BIN_OBJS) builtins.lib

hexdump.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mhexdump.obj: hexdump.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ hexdump.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

