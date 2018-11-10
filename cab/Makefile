
BINARIES=cab.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdcab.pdb
LINKPDB=/Pdb:cab.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCAB_VER_MAJOR=$(CAB_VER_MAJOR) -DCAB_VER_MINOR=$(CAB_VER_MINOR)

BIN_OBJS=\
	 cab.obj         \

MOD_OBJS=\
	 mod_cab.obj     \

compile: $(BIN_OBJS) builtins.lib

cab.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(CAB_VER_MAJOR).$(CAB_VER_MINOR) $(LINKPDB) -out:$@

mod_cab.obj: cab.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ cab.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

