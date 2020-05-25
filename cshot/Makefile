
BINARIES=cshot.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:cshot.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCSHOT_VER_MAJOR=$(CSHOT_VER_MAJOR) -DCSHOT_VER_MINOR=$(CSHOT_VER_MINOR)

BIN_OBJS=\
	 cshot.obj        \

MOD_OBJS=\
	 mod_cshot.obj    \

compile: $(BIN_OBJS) builtins.lib

cshot.exe: $(BIN_OBJS) ..\lib\yorilib.lib ..\crt\yoricrt.lib
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(CSHOT_VER_MAJOR).$(CSHOT_VER_MINOR) $(LINKPDB) -out:$@

mod_cshot.obj: cshot.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ cshot.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

