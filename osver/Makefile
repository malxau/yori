
BINARIES=osver.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:osver.pdb

BIN_OBJS=\
	 osver.obj        \

MOD_OBJS=\
	 mosver.obj    \

compile: $(BIN_OBJS) builtins.lib

osver.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mosver.obj: osver.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ osver.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

