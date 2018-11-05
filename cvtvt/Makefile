
BINARIES=cvtvt.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdcvtvt.pdb
LINKPDB=/Pdb:cvtvt.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCVTVT_VER_MAJOR=$(CVTVT_VER_MAJOR) -DCVTVT_VER_MINOR=$(CVTVT_VER_MINOR)

OBJS=\
	 html.obj        \
	 main.obj        \

compile: $(OBJS)

cvtvt.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(CVTVT_VER_MAJOR).$(CVTVT_VER_MINOR) $(LINKPDB) -out:$@

