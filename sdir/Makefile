
BINARIES=sdir.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdsdir.pdb
LINKPDB=/Pdb:sdir.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DSDIR_VER_MAJOR=$(SDIR_VER_MAJOR) -DSDIR_VER_MINOR=$(SDIR_VER_MINOR)

OBJS=callbacks.obj color.obj display.obj init.obj sdir.obj usage.obj utils.obj

compile: $(OBJS)

sdir.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(ENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(SDIR_VER_MAJOR).$(SDIR_VER_MINOR) $(LINKPDB) -out:$@

