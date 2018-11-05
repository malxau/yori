
BINARIES=hilite.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdhilite.pdb
LINKPDB=/Pdb:hilite.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DHILITE_VER_MAJOR=$(HILITE_VER_MAJOR) -DHILITE_VER_MINOR=$(HILITE_VER_MINOR)

OBJS=\
	 hilite.obj         \

compile: $(OBJS)

hilite.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(HILITE_VER_MAJOR).$(HILITE_VER_MINOR) $(LINKPDB) -out:$@

