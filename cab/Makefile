
link: cab.exe

install:
	@copy cab.exe $(BINDIR) >NUL
	@if exist cab.pdb copy cab.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdcab.pdb
LINKPDB=/Pdb:cab.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCAB_VER_MAJOR=$(CAB_VER_MAJOR) -DCAB_VER_MINOR=$(CAB_VER_MINOR)

OBJS=\
	 cab.obj         \

compile: $(OBJS)

cab.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(CAB_VER_MAJOR).$(CAB_VER_MINOR) $(LINKPDB) -out:$@

