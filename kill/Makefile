
BINARIES=ykill.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ykill.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DKILL_VER_MAJOR=$(KILL_VER_MAJOR) -DKILL_VER_MINOR=$(KILL_VER_MINOR)

BIN_OBJS=\
	 kill.obj         \

MOD_OBJS=\
	 mod_kill.obj     \

compile: $(BIN_OBJS) builtins.lib

ykill.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(KILL_VER_MAJOR).$(KILL_VER_MINOR) $(LINKPDB) -out:$@

mod_kill.obj: kill.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ kill.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


