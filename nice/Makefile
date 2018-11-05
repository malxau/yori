
BINARIES=nice.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdnice.pdb
LINKPDB=/Pdb:nice.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DNICE_VER_MAJOR=$(NICE_VER_MAJOR) -DNICE_VER_MINOR=$(NICE_VER_MINOR)

OBJS=\
	 nice.obj         \

compile: $(OBJS)

nice.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(NICE_VER_MAJOR).$(NICE_VER_MINOR) $(LINKPDB) -out:$@

