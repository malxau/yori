
BINARIES=ymore.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdymore.pdb
LINKPDB=/Pdb:ymore.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DMORE_VER_MAJOR=$(MORE_VER_MAJOR) -DMORE_VER_MINOR=$(MORE_VER_MINOR)

OBJS=\
	 ingest.obj       \
	 init.obj         \
	 more.obj         \
	 viewport.obj     \

compile: $(OBJS)

ymore.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(MORE_VER_MAJOR).$(MORE_VER_MINOR) $(LINKPDB) -out:$@

