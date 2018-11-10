
BINARIES=ycompact.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdycompact.pdb
LINKPDB=/Pdb:ycompact.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCOMPACT_VER_MAJOR=$(COMPACT_VER_MAJOR) -DCOMPACT_VER_MINOR=$(COMPACT_VER_MINOR)

BIN_OBJS=\
	 compact.obj         \

MOD_OBJS=\
	 mod_compact.obj     \

compile: $(BIN_OBJS) builtins.lib

ycompact.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(COMPACT_VER_MAJOR).$(COMPACT_VER_MINOR) $(LINKPDB) -out:$@

mod_compact.obj: compact.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ compact.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

