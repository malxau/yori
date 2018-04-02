
link: yget.exe

install:
	@copy yget.exe $(BINDIR) >NUL
	@if exist yget.pdb copy yget.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdyget.pdb
LINKPDB=/Pdb:yget.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DGET_VER_MAJOR=$(GET_VER_MAJOR) -DGET_VER_MINOR=$(GET_VER_MINOR)

OBJS=\
	 get.obj         \

compile: $(OBJS)

yget.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(GET_VER_MAJOR).$(GET_VER_MINOR) $(LINKPDB) -out:$@

