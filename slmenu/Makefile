
BINARIES=slmenu.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:slmenu.pdb

BIN_OBJS=\
	 slmenu.obj         \

MOD_OBJS=\
	 mslmenu.obj     \

compile: $(BIN_OBJS) builtins.lib

slmenu.exe: $(BIN_OBJS) $(YORILIBS) $(YORIWIN) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIWIN) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mslmenu.obj: slmenu.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ slmenu.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
