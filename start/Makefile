
BINARIES=ystart.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdystart.pdb
LINKPDB=/Pdb:ystart.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DSTART_VER_MAJOR=$(START_VER_MAJOR) -DSTART_VER_MINOR=$(START_VER_MINOR)

BIN_OBJS=\
	 start.obj         \

MOD_OBJS=\
	 mod_start.obj     \

compile: $(BIN_OBJS) builtins.lib

ystart.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(START_VER_MAJOR).$(START_VER_MINOR) $(LINKPDB) -out:$@

mod_start.obj: start.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ start.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

