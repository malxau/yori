
BINARIES=ymount.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(MOUNT_VER_MAJOR)
YORI_BIN_VER_MINOR=$(MOUNT_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ymount.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DMOUNT_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DMOUNT_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 mount.obj         \

MOD_OBJS=\
	 mod_mount.obj     \

compile: $(BIN_OBJS) builtins.lib

ymount.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_mount.obj: mount.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ mount.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
