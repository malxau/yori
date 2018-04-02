

link: sdir.exe

install:
	@copy sdir.exe $(BINDIR) >NUL
	@if exist sdir.pdb copy sdir.pdb $(SYMDIR) >NUL

SDIR_BUILD_ID=0

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdsdir.pdb
LINKPDB=/Pdb:sdir.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DSDIR_VER_MAJOR=$(SDIR_VER_MAJOR) -DSDIR_VER_MINOR=$(SDIR_VER_MINOR) -DSDIR_BUILD_ID=$(SDIR_BUILD_ID)

OBJS=callbacks.obj color.obj display.obj init.obj sdir.obj usage.obj utils.obj

compile: $(OBJS)

sdir.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(ENTRY) $(OBJS) $(LIBS) $(CRTLIB) advapi32.lib ..\lib\yorilib.lib -version:$(SDIR_VER_MAJOR).$(SDIR_VER_MINOR) $(LINKPDB) -out:$@

