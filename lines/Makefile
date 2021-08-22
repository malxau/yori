
BINARIES=lines.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(LINES_VER_MAJOR)
YORI_BIN_VER_MINOR=$(LINES_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:lines.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DLINES_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DLINES_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 lines.obj         \

MOD_OBJS=\
	 mod_lines.obj     \

compile: $(BIN_OBJS) builtins.lib

lines.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_lines.obj: lines.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ lines.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

