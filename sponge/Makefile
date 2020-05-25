
BINARIES=ysponge.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ysponge.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DSPONGE_VER_MAJOR=$(SPONGE_VER_MAJOR) -DSPONGE_VER_MINOR=$(SPONGE_VER_MINOR)

BIN_OBJS=\
	 sponge.obj         \

MOD_OBJS=\
	 mod_sponge.obj     \

compile: $(BIN_OBJS) builtins.lib

ysponge.exe: $(BIN_OBJS) ..\lib\yorilib.lib ..\crt\yoricrt.lib
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(SPONGE_VER_MAJOR).$(SPONGE_VER_MINOR) $(LINKPDB) -out:$@

mod_sponge.obj: sponge.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ sponge.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

