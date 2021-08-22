
BINARIES=winpos.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(WINPOS_VER_MAJOR)
YORI_BIN_VER_MINOR=$(WINPOS_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:winpos.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DWINPOS_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DWINPOS_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 winpos.obj         \

MOD_OBJS=\
	 mod_winpos.obj     \

compile: $(BIN_OBJS) builtins.lib

winpos.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_winpos.obj: winpos.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ winpos.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
