
BINARIES=ymove.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ymove.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DMOVE_VER_MAJOR=$(MOVE_VER_MAJOR) -DMOVE_VER_MINOR=$(MOVE_VER_MINOR)

BIN_OBJS=\
	 move.obj         \

MOD_OBJS=\
	 mod_move.obj     \

compile: $(BIN_OBJS) builtins.lib

ymove.exe: $(BIN_OBJS)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(MOVE_VER_MAJOR).$(MOVE_VER_MINOR) $(LINKPDB) -out:$@

mod_move.obj: move.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ move.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
