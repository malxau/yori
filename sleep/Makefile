
BINARIES=sleep.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:sleep.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DSLEEP_VER_MAJOR=$(SLEEP_VER_MAJOR) -DSLEEP_VER_MINOR=$(SLEEP_VER_MINOR)

BIN_OBJS=\
	 sleep.obj        \

MOD_OBJS=\
	 mod_sleep.obj    \

compile: $(BIN_OBJS) builtins.lib

sleep.exe: $(BIN_OBJS) ..\lib\yorilib.lib ..\crt\yoricrt.lib
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(SLEEP_VER_MAJOR).$(SLEEP_VER_MINOR) $(LINKPDB) -out:$@

mod_sleep.obj: sleep.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ sleep.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

