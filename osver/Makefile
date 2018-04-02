
link: osver.exe

install:
	@copy osver.exe $(BINDIR) >NUL
	@if exist osver.pdb copy osver.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdosver.pdb
LINKPDB=/Pdb:osver.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DOSVER_VER_MAJOR=$(OSVER_VER_MAJOR) -DOSVER_VER_MINOR=$(OSVER_VER_MINOR)

OBJS=\
	 osver.obj        \

compile: $(OBJS)

osver.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(OSVER_VER_MAJOR).$(OSVER_VER_MINOR) $(LINKPDB) -out:$@

