
BINARIES=readline.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(READLINE_VER_MAJOR)
YORI_BIN_VER_MINOR=$(READLINE_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:readline.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DREADLINE_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DREADLINE_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 readline.obj         \

MOD_OBJS=\
	 mod_readline.obj     \

compile: $(BIN_OBJS) builtins.lib

readline.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_readline.obj: readline.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ readline.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
