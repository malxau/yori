
link: ycls.exe

install:
	@copy ycls.exe $(BINDIR) >NUL
	@if exist ycls.pdb copy ycls.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdycls.pdb
LINKPDB=/Pdb:ycls.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCLS_VER_MAJOR=$(CLS_VER_MAJOR) -DCLS_VER_MINOR=$(CLS_VER_MINOR)

OBJS=\
	 cls.obj         \

compile: $(OBJS)

ycls.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(CLS_VER_MAJOR).$(CLS_VER_MINOR) $(LINKPDB) -out:$@

