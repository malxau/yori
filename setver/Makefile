
BINARIES=setver.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdsetver.pdb
LINKPDB=/Pdb:setver.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DSETVER_VER_MAJOR=$(SETVER_VER_MAJOR) -DSETVER_VER_MINOR=$(SETVER_VER_MINOR)

OBJS=\
	 setver.obj         \

compile: $(OBJS)

setver.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(SETVER_VER_MAJOR).$(SETVER_VER_MINOR) $(LINKPDB) -out:$@

