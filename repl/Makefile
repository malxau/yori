
BINARIES=repl.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:repl.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DREPL_VER_MAJOR=$(REPL_VER_MAJOR) -DREPL_VER_MINOR=$(REPL_VER_MINOR)

BIN_OBJS=\
	 repl.obj         \

MOD_OBJS=\
	 mod_repl.obj     \

compile: $(BIN_OBJS) builtins.lib

repl.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(REPL_VER_MAJOR).$(REPL_VER_MINOR) $(LINKPDB) -out:$@

mod_repl.obj: repl.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ repl.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
