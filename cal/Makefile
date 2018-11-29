
BINARIES=ycal.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdycal.pdb
LINKPDB=/Pdb:ycal.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCAL_VER_MAJOR=$(CAL_VER_MAJOR) -DCAL_VER_MINOR=$(CAL_VER_MINOR)

BIN_OBJS=\
	 cal.obj         \

MOD_OBJS=\
	 mod_cal.obj     \

compile: $(BIN_OBJS) builtins.lib

ycal.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(CAL_VER_MAJOR).$(CAL_VER_MINOR) $(LINKPDB) -out:$@

mod_cal.obj: cal.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ cal.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


