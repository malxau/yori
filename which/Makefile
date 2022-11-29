
BINARIES=which.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(WHICH_VER_MAJOR)
YORI_BIN_VER_MINOR=$(WHICH_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:which.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DWHICH_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DWHICH_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 which.obj         \
	 resource.obj      \

MOD_OBJS=\
	 mod_which.obj     \

compile: $(BIN_OBJS) builtins.lib

which.exe: $(BIN_OBJS) $(YORILIBS)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_which.obj: which.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ which.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
