
BINARIES=sync.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(SYNC_VER_MAJOR)
YORI_BIN_VER_MINOR=$(SYNC_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:sync.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DSYNC_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DSYNC_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 sync.obj         \

MOD_OBJS=\
	 mod_sync.obj     \

compile: $(BIN_OBJS) builtins.lib

sync.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_sync.obj: sync.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ sync.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
