
BINARIES=vhdtool.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdvhdtool.pdb
LINKPDB=/Pdb:vhdtool.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DVHDTOOL_VER_MAJOR=$(VHDTOOL_VER_MAJOR) -DVHDTOOL_VER_MINOR=$(VHDTOOL_VER_MINOR)

BIN_OBJS=\
	 vhdtool.obj         \

MOD_OBJS=\
	 mod_vhdtool.obj     \

compile: $(BIN_OBJS) builtins.lib

vhdtool.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(VHDTOOL_VER_MAJOR).$(VHDTOOL_VER_MINOR) $(LINKPDB) -out:$@

mod_vhdtool.obj: vhdtool.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ vhdtool.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

