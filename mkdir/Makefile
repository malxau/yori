
BINARIES=ymkdir.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(MKDIR_VER_MAJOR)
YORI_BIN_VER_MINOR=$(MKDIR_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ymkdir.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DMKDIR_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DMKDIR_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 mkdir.obj        \

MOD_OBJS=\
	 mod_mkdir.obj    \

compile: $(BIN_OBJS) builtins.lib

ymkdir.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_mkdir.obj: mkdir.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ mkdir.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
