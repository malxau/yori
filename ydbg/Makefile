
BINARIES=ydbg.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(YDBG_VER_MAJOR)
YORI_BIN_VER_MINOR=$(YDBG_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ydbg.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DYDBG_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DYDBG_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 dbg.obj         \

MOD_OBJS=\
	 mod_dbg.obj     \

compile: $(BIN_OBJS) builtins.lib

ydbg.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_dbg.obj: dbg.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ dbg.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


