
BINARIES=yedit.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(EDIT_VER_MAJOR)
YORI_BIN_VER_MINOR=$(EDIT_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:yedit.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DEDIT_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DEDIT_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 edit.obj         \
	 about.obj        \
	 options.obj      \

MOD_OBJS=\
	 mod_edit.obj     \
	 about.obj        \
	 options.obj      \

compile: $(BIN_OBJS) builtins.lib

yedit.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIWIN) $(YORIDLG) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIWIN) $(YORIDLG) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_edit.obj: edit.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ edit.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
