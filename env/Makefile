
BINARIES=yenv.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:yenv.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DENV_VER_MAJOR=$(ENV_VER_MAJOR) -DENV_VER_MINOR=$(ENV_VER_MINOR)

BIN_OBJS=\
	 env.obj         \

MOD_OBJS=\
	 mod_env.obj     \

compile: $(BIN_OBJS) builtins.lib

yenv.exe: $(BIN_OBJS) ..\lib\yorilib.lib ..\crt\yoricrt.lib
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(ENV_VER_MAJOR).$(ENV_VER_MINOR) $(LINKPDB) -out:$@

mod_env.obj: env.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ env.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


