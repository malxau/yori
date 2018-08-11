
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

OBJS=\
	 ysetup.obj      \

compile: $(OBJS)

ysetup.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib ..\pkglib\yoripkg.lib -version:$(YSETUP_VER_MAJOR).$(YSETUP_VER_MINOR) $(LINKPDB_YSETUP) -out:$@

