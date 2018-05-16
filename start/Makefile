
link: ystart.exe

install:
	@copy ystart.exe $(BINDIR) >NUL
	@if exist ystart.pdb copy ystart.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdystart.pdb
LINKPDB=/Pdb:ystart.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DSTART_VER_MAJOR=$(START_VER_MAJOR) -DSTART_VER_MINOR=$(START_VER_MINOR)

OBJS=\
	 start.obj         \

compile: $(OBJS)

ystart.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(START_VER_MAJOR).$(START_VER_MINOR) $(LINKPDB) -out:$@

