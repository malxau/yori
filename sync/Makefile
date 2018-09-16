
link: sync.exe

install:
	@copy sync.exe $(BINDIR) >NUL
	@if exist sync.pdb copy sync.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdsync.pdb
LINKPDB=/Pdb:sync.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DSYNC_VER_MAJOR=$(SYNC_VER_MAJOR) -DSYNC_VER_MINOR=$(SYNC_VER_MINOR)

OBJS=\
	 sync.obj         \

compile: $(OBJS)

sync.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(SYNC_VER_MAJOR).$(SYNC_VER_MINOR) $(LINKPDB) -out:$@

