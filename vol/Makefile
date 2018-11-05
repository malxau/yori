
BINARIES=yvol.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdyvol.pdb
LINKPDB=/Pdb:yvol.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DVOL_VER_MAJOR=$(VOL_VER_MAJOR) -DVOL_VER_MINOR=$(VOL_VER_MINOR)

OBJS=\
	 vol.obj        \

compile: $(OBJS)

yvol.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(VOL_VER_MAJOR).$(VOL_VER_MINOR) $(LINKPDB) -out:$@

