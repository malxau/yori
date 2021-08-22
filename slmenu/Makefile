
BINARIES=slmenu.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(SLMENU_VER_MAJOR)
YORI_BIN_VER_MINOR=$(SLMENU_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:slmenu.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DSLMENU_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DSLMENU_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 slmenu.obj         \

MOD_OBJS=\
	 mod_slmenu.obj     \

compile: $(BIN_OBJS) builtins.lib

slmenu.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIWIN) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIWIN) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_slmenu.obj: slmenu.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ slmenu.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
