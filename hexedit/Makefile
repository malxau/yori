
BINARIES=yhexedit.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:yhexedit.pdb
!ENDIF

BIN_OBJS=\
	 hexedit.obj         \

MOD_OBJS=\
	 mod_hexedit.obj     \

compile: $(BIN_OBJS) builtins.lib

yhexedit.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIWIN) $(YORIDLG) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIWIN) $(YORIDLG) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_hexedit.obj: hexedit.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ hexedit.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
