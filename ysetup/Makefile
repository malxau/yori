
link: ysetup.exe

install:
	@copy ysetup.exe $(BINDIR) >NUL
	@if exist ysetup.pdb copy ysetup.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

CFLAGS=$(CFLAGS) -DYSETUP_VER_MAJOR=$(YSETUP_VER_MAJOR) -DYSETUP_VER_MINOR=$(YSETUP_VER_MINOR) -I..\pkglib

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdysetup.pdb
LINKPDB=/Pdb:ysetup.pdb
!ENDIF

# This needs UI, Cabinet, WinInet, Shortcuts...realistically, pre-NT4
# won't work here
LIBS=$(LIBS) user32.lib advapi32.lib
LDFLAGS_NT4UP=$(LDFLAGS:3.10=4.0)

OBJS=\
	 resource.obj    \
	 ysetup.obj      \

compile: $(OBJS)

resource.obj: resource.rc
	@echo resource.rc
	@rc -r resource.rc
	@if exist resource.obj erase resource.obj
	@if not exist resource.obj ren resource.res resource.obj

ysetup.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS_NT4UP) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib ..\pkglib\yoripkg.lib -version:$(YSETUP_VER_MAJOR).$(YSETUP_VER_MINOR) $(LINKPDB_YSETUP) -out:$@

