
BINARIES=scut.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(SCUT_VER_MAJOR)
YORI_BIN_VER_MINOR=$(SCUT_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:scut.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DSCUT_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DSCUT_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 scut.obj         \

MOD_OBJS=\
	 mod_scut.obj     \

compile: $(BIN_OBJS) builtins.lib

scut.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_scut.obj: scut.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ scut.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
