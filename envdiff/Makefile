
BINARIES=envdiff.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(ENVDIFF_VER_MAJOR)
YORI_BIN_VER_MINOR=$(ENVDIFF_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:envdiff.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DENVDIFF_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DENVDIFF_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 envdiff.obj        \

MOD_OBJS=\
	 mod_envdiff.obj    \

compile: $(BIN_OBJS) builtins.lib

envdiff.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_envdiff.obj: envdiff.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ envdiff.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

