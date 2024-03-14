
BINARIES=yhexedit.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:yhexedit.pdb

BIN_OBJS=\
	 hexedit.obj         \

MOD_OBJS=\
	 mhexedit.obj     \

compile: $(BIN_OBJS) builtins.lib

yhexedit.exe: $(BIN_OBJS) $(YORILIBS) $(YORIWIN) $(YORIDLG) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIWIN) $(YORIDLG) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mhexedit.obj: hexedit.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ hexedit.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
