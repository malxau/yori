
BINARIES=which.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdwhich.pdb
LINKPDB=/Pdb:which.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DWHICH_VER_MAJOR=$(WHICH_VER_MAJOR) -DWHICH_VER_MINOR=$(WHICH_VER_MINOR)

OBJS=\
	 which.obj         \

compile: $(OBJS)

which.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(WHICH_VER_MAJOR).$(WHICH_VER_MINOR) $(LINKPDB) -out:$@

