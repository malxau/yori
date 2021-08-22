
BINARIES=fscmp.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(FSCMP_VER_MAJOR)
YORI_BIN_VER_MINOR=$(FSCMP_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:fscmp.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DFSCMP_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DFSCMP_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 fscmp.obj        \

MOD_OBJS=\
	 mod_fscmp.obj    \

compile: $(BIN_OBJS) builtins.lib

fscmp.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_fscmp.obj: fscmp.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ fscmp.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
