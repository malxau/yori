
BINARIES=ymem.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdymem.pdb
LINKPDB=/Pdb:ymem.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DMEM_VER_MAJOR=$(MEM_VER_MAJOR) -DMEM_VER_MINOR=$(MEM_VER_MINOR)

BIN_OBJS=\
	 mem.obj         \

MOD_OBJS=\
	 mod_mem.obj     \

compile: $(BIN_OBJS) builtins.lib

ymem.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(MEM_VER_MAJOR).$(MEM_VER_MINOR) $(LINKPDB) -out:$@

mod_mem.obj: mem.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ mem.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
