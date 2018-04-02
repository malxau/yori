
link: ymove.exe

install:
	@copy ymove.exe $(BINDIR) >NUL
	@if exist ymove.pdb copy ymove.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdymove.pdb
LINKPDB=/Pdb:ymove.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DMOVE_VER_MAJOR=$(MOVE_VER_MAJOR) -DMOVE_VER_MINOR=$(MOVE_VER_MINOR)

OBJS=\
	 move.obj         \

compile: $(OBJS)

ymove.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) advapi32.lib ..\lib\yorilib.lib -version:$(MOVE_VER_MAJOR).$(MOVE_VER_MINOR) $(LINKPDB) -out:$@

