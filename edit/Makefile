
BINARIES=yedit.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:yedit.pdb
!ENDIF

BIN_OBJS=\
	 edit.obj         \
	 options.obj      \

MOD_OBJS=\
	 mod_edit.obj     \
	 options.obj      \

compile: $(BIN_OBJS) builtins.lib

yedit.exe: $(BIN_OBJS) $(YORILIBS) $(YORIWIN) $(YORIDLG) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIWIN) $(YORIDLG) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_edit.obj: edit.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ edit.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
