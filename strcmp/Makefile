
BINARIES=strcmp.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:strcmp.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DSTRCMP_VER_MAJOR=$(STRCMP_VER_MAJOR) -DSTRCMP_VER_MINOR=$(STRCMP_VER_MINOR)

BIN_OBJS=\
	 strcmp.obj        \

MOD_OBJS=\
	 mod_strcmp.obj    \

compile: $(BIN_OBJS) builtins.lib

strcmp.exe: $(BIN_OBJS) ..\lib\yorilib.lib ..\crt\yoricrt.lib
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(STRCMP_VER_MAJOR).$(STRCMP_VER_MINOR) $(LINKPDB) -out:$@

mod_strcmp.obj: strcmp.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ strcmp.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
