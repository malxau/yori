
BINARIES=fscmp.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdfscmp.pdb
LINKPDB=/Pdb:fscmp.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DFSCMP_VER_MAJOR=$(FSCMP_VER_MAJOR) -DFSCMP_VER_MINOR=$(FSCMP_VER_MINOR)

BIN_OBJS=\
	 fscmp.obj        \

MOD_OBJS=\
	 mod_fscmp.obj    \

compile: $(BIN_OBJS) builtins.lib

fscmp.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(FSCMP_VER_MAJOR).$(FSCMP_VER_MINOR) $(LINKPDB) -out:$@

mod_fscmp.obj: fscmp.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ fscmp.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


