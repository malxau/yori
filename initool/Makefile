
BINARIES=initool.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(INITOOL_VER_MAJOR)
YORI_BIN_VER_MINOR=$(INITOOL_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:initool.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DINITOOL_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DINITOOL_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 initool.obj         \

MOD_OBJS=\
	 mod_initool.obj     \

compile: $(BIN_OBJS) builtins.lib

initool.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_initool.obj: initool.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ initool.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

