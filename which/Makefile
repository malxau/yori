
BINARIES=which.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:which.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DWHICH_VER_MAJOR=$(WHICH_VER_MAJOR) -DWHICH_VER_MINOR=$(WHICH_VER_MINOR)

BIN_OBJS=\
	 which.obj         \

MOD_OBJS=\
	 mod_which.obj     \

compile: $(BIN_OBJS) builtins.lib

which.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) -version:$(WHICH_VER_MAJOR).$(WHICH_VER_MINOR) $(LINKPDB) -out:$@

mod_which.obj: which.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ which.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
