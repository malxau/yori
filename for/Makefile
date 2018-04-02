
link: yfor.exe

install:
	@copy yfor.exe $(BINDIR) >NUL
	@if exist yfor.pdb copy yfor.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdyfor.pdb
LINKPDB=/Pdb:yfor.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DFOR_VER_MAJOR=$(FOR_VER_MAJOR) -DFOR_VER_MINOR=$(FOR_VER_MINOR)

OBJS=\
	 for.obj        \

compile: $(OBJS)

yfor.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(FOR_VER_MAJOR).$(FOR_VER_MINOR) $(LINKPDB) -out:$@

