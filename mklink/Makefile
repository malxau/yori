
BINARIES=ymklink.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ymklink.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DMKLINK_VER_MAJOR=$(MKLINK_VER_MAJOR) -DMKLINK_VER_MINOR=$(MKLINK_VER_MINOR)

BIN_OBJS=\
	 mklink.obj         \

MOD_OBJS=\
	 mod_mklink.obj     \

compile: $(BIN_OBJS) builtins.lib

ymklink.exe: $(BIN_OBJS) ..\lib\yorilib.lib ..\crt\yoricrt.lib
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(MKLINK_VER_MAJOR).$(MKLINK_VER_MINOR) $(LINKPDB) -out:$@

mod_mklink.obj: mklink.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ mklink.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
