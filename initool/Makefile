
link: initool.exe

install:
	@copy initool.exe $(BINDIR) >NUL
	@if exist initool.pdb copy initool.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdinitool.pdb
LINKPDB=/Pdb:initool.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DINITOOL_VER_MAJOR=$(INITOOL_VER_MAJOR) -DINITOOL_VER_MINOR=$(INITOOL_VER_MINOR)

OBJS=\
	 initool.obj         \

compile: $(OBJS)

initool.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(INITOOL_VER_MAJOR).$(INITOOL_VER_MINOR) $(LINKPDB) -out:$@

