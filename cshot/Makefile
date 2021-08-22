
BINARIES=cshot.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(CSHOT_VER_MAJOR)
YORI_BIN_VER_MINOR=$(CSHOT_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:cshot.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCSHOT_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DCSHOT_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 cshot.obj        \

MOD_OBJS=\
	 mod_cshot.obj    \

compile: $(BIN_OBJS) builtins.lib

cshot.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_cshot.obj: cshot.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ cshot.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

