
BINARIES=yps.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(PS_VER_MAJOR)
YORI_BIN_VER_MINOR=$(PS_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:yps.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DPS_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DPS_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 ps.obj         \

MOD_OBJS=\
	 mod_ps.obj     \

compile: $(BIN_OBJS) builtins.lib

yps.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_ps.obj: ps.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ ps.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
