
BINARIES=timethis.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:timethis.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DTIMETHIS_VER_MAJOR=$(TIMETHIS_VER_MAJOR) -DTIMETHIS_VER_MINOR=$(TIMETHIS_VER_MINOR)

BIN_OBJS=\
	 timethis.obj         \

MOD_OBJS=\
	 mod_time.obj     \

compile: $(BIN_OBJS) builtins.lib

timethis.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) -version:$(TIMETHIS_VER_MAJOR).$(TIMETHIS_VER_MINOR) $(LINKPDB) -out:$@

mod_time.obj: timethis.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ timethis.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

