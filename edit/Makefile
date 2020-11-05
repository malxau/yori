
BINARIES=yedit.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:yedit.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCO_VER_MAJOR=$(CO_VER_MAJOR) -DCO_VER_MINOR=$(CO_VER_MINOR)

BIN_OBJS=\
	 edit.obj         \
	 about.obj        \
	 options.obj      \

MOD_OBJS=\
	 mod_edit.obj     \
	 about.obj        \
	 options.obj      \

compile: $(BIN_OBJS) builtins.lib

yedit.exe: $(BIN_OBJS) ..\lib\yorilib.lib ..\crt\yoricrt.lib ..\libwin\yoriwin.lib ..\libdlg\yoridlg.lib
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib ..\libwin\yoriwin.lib ..\libdlg\yoridlg.lib -version:$(CO_VER_MAJOR).$(CO_VER_MINOR) $(LINKPDB) -out:$@

mod_edit.obj: edit.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ edit.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
