
BINARIES=yrmdir.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:yrmdir.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DRMDIR_VER_MAJOR=$(RMDIR_VER_MAJOR) -DRMDIR_VER_MINOR=$(RMDIR_VER_MINOR)

BIN_OBJS=\
	 rmdir.obj        \

MOD_OBJS=\
	 mod_rmdir.obj    \

compile: $(BIN_OBJS) builtins.lib

yrmdir.exe: $(BIN_OBJS) ..\lib\yorilib.lib ..\crt\yoricrt.lib
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(RMDIR_VER_MAJOR).$(RMDIR_VER_MINOR) $(LINKPDB) -out:$@

mod_rmdir.obj: rmdir.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ rmdir.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
