
BINARIES=yps.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:yps.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DPS_VER_MAJOR=$(PS_VER_MAJOR) -DPS_VER_MINOR=$(PS_VER_MINOR)

BIN_OBJS=\
	 ps.obj         \

MOD_OBJS=\
	 mod_ps.obj     \

compile: $(BIN_OBJS) builtins.lib

yps.exe: $(BIN_OBJS) ..\lib\yorilib.lib ..\crt\yoricrt.lib
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(PS_VER_MAJOR).$(PS_VER_MINOR) $(LINKPDB) -out:$@

mod_ps.obj: ps.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ ps.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
