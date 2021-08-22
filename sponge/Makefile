
BINARIES=ysponge.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(SPONGE_VER_MAJOR)
YORI_BIN_VER_MINOR=$(SPONGE_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ysponge.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DSPONGE_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DSPONGE_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 sponge.obj         \

MOD_OBJS=\
	 mod_sponge.obj     \

compile: $(BIN_OBJS) builtins.lib

ysponge.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_sponge.obj: sponge.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ sponge.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

