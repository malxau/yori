
BINARIES=ycls.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(CLS_VER_MAJOR)
YORI_BIN_VER_MINOR=$(CLS_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ycls.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCLS_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DCLS_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 cls.obj         \

MOD_OBJS=\
	 mod_cls.obj     \

compile: $(BIN_OBJS) builtins.lib

ycls.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_cls.obj: cls.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ cls.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

