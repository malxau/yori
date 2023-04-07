
BINARIES=ykill.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ykill.pdb

BIN_OBJS=\
	 kill.obj         \

MOD_OBJS=\
	 mod_kill.obj     \

compile: $(BIN_OBJS) builtins.lib

ykill.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_kill.obj: kill.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ kill.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


