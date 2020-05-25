
BINARIES=ypause.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ypause.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DPAUSE_VER_MAJOR=$(PAUSE_VER_MAJOR) -DPAUSE_VER_MINOR=$(PAUSE_VER_MINOR)

BIN_OBJS=\
	 pause.obj         \

MOD_OBJS=\
	 mod_pause.obj     \

compile: $(BIN_OBJS) builtins.lib

ypause.exe: $(BIN_OBJS) ..\lib\yorilib.lib ..\crt\yoricrt.lib
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(PAUSE_VER_MAJOR).$(PAUSE_VER_MINOR) $(LINKPDB) -out:$@

mod_pause.obj: pause.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ pause.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


