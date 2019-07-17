
BINARIES=scut.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:scut.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DSCUT_VER_MAJOR=$(SCUT_VER_MAJOR) -DSCUT_VER_MINOR=$(SCUT_VER_MINOR)

BIN_OBJS=\
	 scut.obj         \

MOD_OBJS=\
	 mod_scut.obj     \

compile: $(BIN_OBJS) builtins.lib

scut.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(SCUT_VER_MAJOR).$(SCUT_VER_MINOR) $(LINKPDB) -out:$@

mod_scut.obj: scut.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ scut.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
