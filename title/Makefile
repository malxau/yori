
BINARIES=ytitle.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ytitle.pdb

BIN_OBJS=\
	 title.obj         \

MOD_OBJS=\
	 mtitle.obj     \

compile: $(BIN_OBJS) builtins.lib

ytitle.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mtitle.obj: title.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ title.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
