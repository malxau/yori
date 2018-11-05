
BINARIES=cshot.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdcshot.pdb
LINKPDB=/Pdb:cshot.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCSHOT_VER_MAJOR=$(CSHOT_VER_MAJOR) -DCSHOT_VER_MINOR=$(CSHOT_VER_MINOR)

OBJS=\
	 cshot.obj        \

compile: $(OBJS)

cshot.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(CSHOT_VER_MAJOR).$(CSHOT_VER_MINOR) $(LINKPDB) -out:$@

