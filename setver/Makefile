
BINARIES=setver.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(SETVER_VER_MAJOR)
YORI_BIN_VER_MINOR=$(SETVER_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:setver.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DSETVER_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DSETVER_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 setver.obj         \

MOD_OBJS=\
	 mod_setver.obj     \

compile: $(BIN_OBJS) builtins.lib

setver.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_setver.obj: setver.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ setver.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
