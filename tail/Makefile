
link: tail.exe

install:
	@copy tail.exe $(BINDIR) >NUL
	@if exist tail.pdb copy tail.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdtail.pdb
LINKPDB=/Pdb:tail.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DTAIL_VER_MAJOR=$(TAIL_VER_MAJOR) -DTAIL_VER_MINOR=$(TAIL_VER_MINOR)

OBJS=\
	 tail.obj         \

compile: $(OBJS)

tail.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(TAIL_VER_MAJOR).$(TAIL_VER_MINOR) $(LINKPDB) -out:$@

