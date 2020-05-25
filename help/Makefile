
BINARIES=yhelp.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:yhelp.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DHELP_VER_MAJOR=$(HELP_VER_MAJOR) -DHELP_VER_MINOR=$(HELP_VER_MINOR)

BIN_OBJS=\
	 help.obj         \

MOD_OBJS=\
	 mod_help.obj     \

compile: $(BIN_OBJS) builtins.lib

yhelp.exe: $(BIN_OBJS) ..\lib\yorilib.lib ..\crt\yoricrt.lib
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(HELP_VER_MAJOR).$(HELP_VER_MINOR) $(LINKPDB) -out:$@

mod_help.obj: help.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ help.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

