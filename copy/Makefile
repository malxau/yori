
BINARIES=ycopy.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdycopy.pdb
LINKPDB=/Pdb:ycopy.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCOPY_VER_MAJOR=$(COPY_VER_MAJOR) -DCOPY_VER_MINOR=$(COPY_VER_MINOR)

BIN_OBJS=\
	 copy.obj         \

MOD_OBJS=\
	 mod_copy.obj     \

compile: $(BIN_OBJS) builtins.lib

ycopy.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(COPY_VER_MAJOR).$(COPY_VER_MINOR) $(LINKPDB) -out:$@

mod_copy.obj: copy.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ copy.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


