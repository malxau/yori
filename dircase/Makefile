
BINARIES=ydircase.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ydircase.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DDIRCASE_VER_MAJOR=$(DIRCASE_VER_MAJOR) -DDIRCASE_VER_MINOR=$(DIRCASE_VER_MINOR)

BIN_OBJS=\
	 dircase.obj         \

MOD_OBJS=\
	 mod_dircase.obj     \

compile: $(BIN_OBJS) builtins.lib

ydircase.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) -version:$(DIRCASE_VER_MAJOR).$(DIRCASE_VER_MINOR) $(LINKPDB) -out:$@

mod_dircase.obj: dircase.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ dircase.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

