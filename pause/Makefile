
BINARIES=ypause.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdypause.pdb
LINKPDB=/Pdb:ypause.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DPAUSE_VER_MAJOR=$(PAUSE_VER_MAJOR) -DPAUSE_VER_MINOR=$(PAUSE_VER_MINOR)

OBJS=\
	 pause.obj         \

compile: $(OBJS)

ypause.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(PAUSE_VER_MAJOR).$(PAUSE_VER_MINOR) $(LINKPDB) -out:$@

