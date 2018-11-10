
BINARIES=sync.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdsync.pdb
LINKPDB=/Pdb:sync.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DSYNC_VER_MAJOR=$(SYNC_VER_MAJOR) -DSYNC_VER_MINOR=$(SYNC_VER_MINOR)

BIN_OBJS=\
	 sync.obj         \

MOD_OBJS=\
	 mod_sync.obj     \

compile: $(BIN_OBJS) builtins.lib

sync.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(SYNC_VER_MAJOR).$(SYNC_VER_MINOR) $(LINKPDB) -out:$@

mod_sync.obj: sync.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ sync.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
