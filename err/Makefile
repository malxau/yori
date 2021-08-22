
BINARIES=yerr.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(ERR_VER_MAJOR)
YORI_BIN_VER_MINOR=$(ERR_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:yerr.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DERR_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DERR_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 err.obj         \

MOD_OBJS=\
	 mod_err.obj     \

compile: $(BIN_OBJS) builtins.lib

yerr.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_err.obj: err.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ err.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
