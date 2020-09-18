
BINARIES=yintcmp.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:yintcmp.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DINTCMP_VER_MAJOR=$(INTCMP_VER_MAJOR) -DINTCMP_VER_MINOR=$(INTCMP_VER_MINOR)

BIN_OBJS=\
	 intcmp.obj        \

MOD_OBJS=\
	 mod_intcmp.obj    \

compile: $(BIN_OBJS) builtins.lib

yintcmp.exe: $(BIN_OBJS) ..\lib\yorilib.lib ..\crt\yoricrt.lib
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(INTCMP_VER_MAJOR).$(INTCMP_VER_MINOR) $(LINKPDB) -out:$@

mod_intcmp.obj: intcmp.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ intcmp.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
