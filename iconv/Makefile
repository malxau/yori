
BINARIES=iconv.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:iconv.pdb

BIN_OBJS=\
	 iconv.obj         \

MOD_OBJS=\
	 miconv.obj     \

compile: $(BIN_OBJS) builtins.lib

iconv.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

miconv.obj: iconv.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ iconv.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
