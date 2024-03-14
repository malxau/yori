
BINARIES=sleep.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:sleep.pdb

BIN_OBJS=\
	 sleep.obj        \

MOD_OBJS=\
	 msleep.obj    \

compile: $(BIN_OBJS) builtins.lib

sleep.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

msleep.obj: sleep.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ sleep.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

