
BINARIES=ydircase.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(DIRCASE_VER_MAJOR)
YORI_BIN_VER_MINOR=$(DIRCASE_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ydircase.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DDIRCASE_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DDIRCASE_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 dircase.obj         \

MOD_OBJS=\
	 mod_dircase.obj     \

compile: $(BIN_OBJS) builtins.lib

ydircase.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_dircase.obj: dircase.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ dircase.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

