
BINARIES=yassoc.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:yassoc.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DASSOC_VER_MAJOR=$(ASSOC_VER_MAJOR) -DASSOC_VER_MINOR=$(ASSOC_VER_MINOR)

BIN_OBJS=\
	 assoc.obj         \

MOD_OBJS=\
	 mod_assoc.obj     \

compile: $(BIN_OBJS) builtins.lib

yassoc.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) -version:$(ASSOC_VER_MAJOR).$(ASSOC_VER_MINOR) $(LINKPDB) -out:$@

mod_assoc.obj: assoc.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ assoc.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


