
BINARIES=ymkdir.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdymkdir.pdb
LINKPDB=/Pdb:ymkdir.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DMKDIR_VER_MAJOR=$(MKDIR_VER_MAJOR) -DMKDIR_VER_MINOR=$(MKDIR_VER_MINOR)

OBJS=\
	 mkdir.obj        \

compile: $(OBJS)

ymkdir.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(MKDIR_VER_MAJOR).$(MKDIR_VER_MINOR) $(LINKPDB) -out:$@

