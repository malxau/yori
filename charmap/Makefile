
BINARIES=ycharmap.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ycharmap.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCHARMAP_VER_MAJOR=$(CHARMAP_VER_MAJOR) -DCHARMAP_VER_MINOR=$(CHARMAP_VER_MINOR)

BIN_OBJS=\
	 charmap.obj         \

MOD_OBJS=\
	 mod_charmap.obj     \

compile: $(BIN_OBJS) builtins.lib

ycharmap.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(CHARMAP_VER_MAJOR).$(CHARMAP_VER_MINOR) $(LINKPDB) -out:$@

mod_charmap.obj: charmap.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ charmap.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

