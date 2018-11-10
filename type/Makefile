
BINARIES=ytype.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdytype.pdb
LINKPDB=/Pdb:ytype.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DTYPE_VER_MAJOR=$(TYPE_VER_MAJOR) -DTYPE_VER_MINOR=$(TYPE_VER_MINOR)

BIN_OBJS=\
	 type.obj         \

MOD_OBJS=\
	 mod_type.obj     \

compile: $(BIN_OBJS) builtins.lib

ytype.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(TYPE_VER_MAJOR).$(TYPE_VER_MINOR) $(LINKPDB) -out:$@

mod_type.obj: type.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ type.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
