
BINARIES=setver.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdsetver.pdb
LINKPDB=/Pdb:setver.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DSETVER_VER_MAJOR=$(SETVER_VER_MAJOR) -DSETVER_VER_MINOR=$(SETVER_VER_MINOR)

BIN_OBJS=\
	 setver.obj         \

MOD_OBJS=\
	 mod_setver.obj     \

compile: $(BIN_OBJS) builtins.lib

setver.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(SETVER_VER_MAJOR).$(SETVER_VER_MINOR) $(LINKPDB) -out:$@

mod_setver.obj: setver.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ setver.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
