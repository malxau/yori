
BINARIES=yspeak.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:yspeak.pdb

BIN_OBJS=\
	 speak.obj         \

MOD_OBJS=\
	 mspeak.obj     \

compile: $(BIN_OBJS) builtins.lib

yspeak.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mspeak.obj: speak.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ speak.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


