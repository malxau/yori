
link: ypath.exe

install:
	@copy ypath.exe $(BINDIR) >NUL
	@if exist ypath.pdb copy ypath.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdypath.pdb
LINKPDB=/Pdb:ypath.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DPATH_VER_MAJOR=$(PATH_VER_MAJOR) -DPATH_VER_MINOR=$(PATH_VER_MINOR)

OBJS=\
	 path.obj        \

compile: $(OBJS)

ypath.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(PATH_VER_MAJOR).$(PATH_VER_MINOR) $(LINKPDB) -out:$@

