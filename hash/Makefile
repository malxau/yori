
BINARIES=yhash.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:yhash.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DHASH_VER_MAJOR=$(HASH_VER_MAJOR) -DHASH_VER_MINOR=$(HASH_VER_MINOR)

BIN_OBJS=\
	 hash.obj         \

MOD_OBJS=\
	 mod_hash.obj     \

compile: $(BIN_OBJS) builtins.lib

yhash.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(HASH_VER_MAJOR).$(HASH_VER_MINOR) $(LINKPDB) -out:$@

mod_hash.obj: hash.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ hash.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

