
link: winpos.exe

install:
	@copy winpos.exe $(BINDIR) >NUL
	@if exist winpos.pdb copy winpos.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdwinpos.pdb
LINKPDB=/Pdb:winpos.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DWINPOS_VER_MAJOR=$(WINPOS_VER_MAJOR) -DWINPOS_VER_MINOR=$(WINPOS_VER_MINOR)

OBJS=\
	 winpos.obj         \

compile: $(OBJS)

winpos.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(WINPOS_VER_MAJOR).$(WINPOS_VER_MINOR) $(LINKPDB) -out:$@

