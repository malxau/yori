
BINARIES=ydir.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ydir.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DDIR_VER_MAJOR=$(DIR_VER_MAJOR) -DDIR_VER_MINOR=$(DIR_VER_MINOR)

BIN_OBJS=\
	 dir.obj         \

MOD_OBJS=\
	 mod_dir.obj     \

compile: $(BIN_OBJS) builtins.lib

ydir.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) -version:$(DIR_VER_MAJOR).$(DIR_VER_MINOR) $(LINKPDB) -out:$@

mod_dir.obj: dir.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ dir.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


