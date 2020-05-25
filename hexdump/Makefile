
BINARIES=hexdump.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:hexdump.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DHEXDUMP_VER_MAJOR=$(HEXDUMP_VER_MAJOR) -DHEXDUMP_VER_MINOR=$(HEXDUMP_VER_MINOR)

BIN_OBJS=\
	 hexdump.obj         \

MOD_OBJS=\
	 mod_hexdump.obj     \

compile: $(BIN_OBJS) builtins.lib

hexdump.exe: $(BIN_OBJS) ..\lib\yorilib.lib ..\crt\yoricrt.lib
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(HEXDUMP_VER_MAJOR).$(HEXDUMP_VER_MINOR) $(LINKPDB) -out:$@

mod_hexdump.obj: hexdump.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ hexdump.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

