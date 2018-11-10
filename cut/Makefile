
BINARIES=ycut.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdycut.pdb
LINKPDB=/Pdb:ycut.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCUT_VER_MAJOR=$(CUT_VER_MAJOR) -DCUT_VER_MINOR=$(CUT_VER_MINOR)

BIN_OBJS=\
	 cut.obj         \

MOD_OBJS=\
	 mod_cut.obj     \

compile: $(BIN_OBJS) builtins.lib

ycut.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(CUT_VER_MAJOR).$(CUT_VER_MINOR) $(LINKPDB) -out:$@

mod_cut.obj: cut.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ cut.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


