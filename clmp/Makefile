
BINARIES=clmp.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdclmp.pdb
LINKPDB=/Pdb:clmp.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCLMP_VER_MAJOR=$(CLMP_VER_MAJOR) -DCLMP_VER_MINOR=$(CLMP_VER_MINOR)

OBJS=\
	 clmp.obj        \

compile: $(OBJS)

clmp.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(CLMP_VER_MAJOR).$(CLMP_VER_MINOR) $(LINKPDB) -out:$@

