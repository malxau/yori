
BINARIES=osver.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:osver.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DOSVER_VER_MAJOR=$(OSVER_VER_MAJOR) -DOSVER_VER_MINOR=$(OSVER_VER_MINOR)

BIN_OBJS=\
	 osver.obj        \

MOD_OBJS=\
	 mod_osver.obj    \

compile: $(BIN_OBJS) builtins.lib

osver.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) -version:$(OSVER_VER_MAJOR).$(OSVER_VER_MINOR) $(LINKPDB) -out:$@

mod_osver.obj: osver.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ osver.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

