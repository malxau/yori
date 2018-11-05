
BINARIES=ydir.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdydir.pdb
LINKPDB=/Pdb:ydir.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DDIR_VER_MAJOR=$(DIR_VER_MAJOR) -DDIR_VER_MINOR=$(DIR_VER_MINOR)

OBJS=\
	 dir.obj         \

compile: $(OBJS)

ydir.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(DIR_VER_MAJOR).$(DIR_VER_MINOR) $(LINKPDB) -out:$@

