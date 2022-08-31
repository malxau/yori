
BINARIES=yregedit.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:yregedit.pdb
!ENDIF

BIN_OBJS=\
	 numedit.obj         \
	 regedit.obj         \
	 stredit.obj         \


MOD_OBJS=\
	 mod_regedit.obj     \
	 numedit.obj         \
	 stredit.obj         \

compile: $(BIN_OBJS) builtins.lib

yregedit.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIWIN) $(YORIDLG) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIWIN) $(YORIDLG) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_regedit.obj: regedit.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ regedit.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
