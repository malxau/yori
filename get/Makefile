
BINARIES=yget.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:yget.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DGET_VER_MAJOR=$(GET_VER_MAJOR) -DGET_VER_MINOR=$(GET_VER_MINOR)

BIN_OBJS=\
	 get.obj         \

MOD_OBJS=\
	 mod_get.obj     \

compile: $(BIN_OBJS) builtins.lib

yget.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(GET_VER_MAJOR).$(GET_VER_MINOR) $(LINKPDB) -out:$@

mod_get.obj: get.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ get.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@



