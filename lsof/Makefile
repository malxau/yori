
BINARIES=lsof.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdlsof.pdb
LINKPDB=/Pdb:lsof.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DLSOF_VER_MAJOR=$(LSOF_VER_MAJOR) -DLSOF_VER_MINOR=$(LSOF_VER_MINOR)

OBJS=\
	 lsof.obj         \

compile: $(OBJS)

lsof.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(LSOF_VER_MAJOR).$(LSOF_VER_MINOR) $(LINKPDB) -out:$@

