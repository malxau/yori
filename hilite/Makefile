
BINARIES=hilite.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:hilite.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DHILITE_VER_MAJOR=$(HILITE_VER_MAJOR) -DHILITE_VER_MINOR=$(HILITE_VER_MINOR)

BIN_OBJS=\
	 hilite.obj         \

MOD_OBJS=\
	 mod_hilite.obj     \

compile: $(BIN_OBJS) builtins.lib

hilite.exe: $(BIN_OBJS) ..\lib\yorilib.lib ..\crt\yoricrt.lib
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(HILITE_VER_MAJOR).$(HILITE_VER_MINOR) $(LINKPDB) -out:$@

mod_hilite.obj: hilite.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ hilite.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


