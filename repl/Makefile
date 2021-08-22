
BINARIES=repl.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(REPL_VER_MAJOR)
YORI_BIN_VER_MINOR=$(REPL_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:repl.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DREPL_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DREPL_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 repl.obj         \

MOD_OBJS=\
	 mod_repl.obj     \

compile: $(BIN_OBJS) builtins.lib

repl.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_repl.obj: repl.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ repl.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
