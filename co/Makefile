
BINARIES=yco.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:yco.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCO_VER_MAJOR=$(CO_VER_MAJOR) -DCO_VER_MINOR=$(CO_VER_MINOR)

BIN_OBJS=\
	 co.obj         \

MOD_OBJS=\
	 mod_co.obj     \

compile: $(BIN_OBJS) builtins.lib

yco.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIWIN) $(YORIDLG)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIWIN) $(YORIDLG) -version:$(CO_VER_MAJOR).$(CO_VER_MINOR) $(LINKPDB) -out:$@

mod_co.obj: co.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ co.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
