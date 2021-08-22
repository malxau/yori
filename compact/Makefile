
BINARIES=ycompact.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(COMPACT_VER_MAJOR)
YORI_BIN_VER_MINOR=$(COMPACT_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ycompact.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCOMPACT_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DCOMPACT_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 compact.obj         \

MOD_OBJS=\
	 mod_compact.obj     \

compile: $(BIN_OBJS) builtins.lib

ycompact.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_compact.obj: compact.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ compact.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

