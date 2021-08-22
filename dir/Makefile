
BINARIES=ydir.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(DIR_VER_MAJOR)
YORI_BIN_VER_MINOR=$(DIR_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ydir.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DDIR_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DDIR_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 dir.obj         \

MOD_OBJS=\
	 mod_dir.obj     \

compile: $(BIN_OBJS) builtins.lib

ydir.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_dir.obj: dir.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ dir.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


