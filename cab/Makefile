
BINARIES=cab.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(CAB_VER_MAJOR)
YORI_BIN_VER_MINOR=$(CAB_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:cab.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCAB_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DCAB_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 cab.obj         \

MOD_OBJS=\
	 mod_cab.obj     \

compile: $(BIN_OBJS) builtins.lib

cab.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) $(FDILIB) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_cab.obj: cab.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ cab.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

