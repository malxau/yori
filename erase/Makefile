
BINARIES=yerase.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:yerase.pdb

BIN_OBJS=\
	 erase.obj        \

MOD_OBJS=\
	 merase.obj    \

compile: $(BIN_OBJS) builtins.lib

yerase.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

merase.obj: erase.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ erase.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
