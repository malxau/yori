
BINARIES=finfo.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdfinfo.pdb
LINKPDB=/Pdb:finfo.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DFINFO_VER_MAJOR=$(FINFO_VER_MAJOR) -DFINFO_VER_MINOR=$(FINFO_VER_MINOR)

OBJS=\
	 finfo.obj         \

compile: $(OBJS)

finfo.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(FINFO_VER_MAJOR).$(FINFO_VER_MINOR) $(LINKPDB) -out:$@

