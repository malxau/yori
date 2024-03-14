
BINARIES=yco.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:yco.pdb

BIN_OBJS=\
	 co.obj         \

MOD_OBJS=\
	 mco.obj     \

compile: $(BIN_OBJS) builtins.lib

yco.exe: $(BIN_OBJS) $(YORILIBS) $(YORIWIN) $(YORIDLG) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIWIN) $(YORIDLG) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mco.obj: co.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ co.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
