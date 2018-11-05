
BINARIES=lines.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdlines.pdb
LINKPDB=/Pdb:lines.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DLINES_VER_MAJOR=$(LINES_VER_MAJOR) -DLINES_VER_MINOR=$(LINES_VER_MINOR)

OBJS=\
	 lines.obj         \

compile: $(OBJS)

lines.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(LINES_VER_MAJOR).$(LINES_VER_MINOR) $(LINKPDB) -out:$@

