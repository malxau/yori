
link: ycut.exe

install:
	@copy ycut.exe $(BINDIR) >NUL
	@if exist ycut.pdb copy ycut.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdycut.pdb
LINKPDB=/Pdb:ycut.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCUT_VER_MAJOR=$(CUT_VER_MAJOR) -DCUT_VER_MINOR=$(CUT_VER_MINOR)

OBJS=\
	 cut.obj         \

compile: $(OBJS)

ycut.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(CUT_VER_MAJOR).$(CUT_VER_MINOR) $(LINKPDB) -out:$@

