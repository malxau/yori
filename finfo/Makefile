
BINARIES=finfo.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(FINFO_VER_MAJOR)
YORI_BIN_VER_MINOR=$(FINFO_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:finfo.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DFINFO_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DFINFO_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 finfo.obj         \

MOD_OBJS=\
	 mod_finfo.obj     \

compile: $(BIN_OBJS) builtins.lib

finfo.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_finfo.obj: finfo.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ finfo.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
