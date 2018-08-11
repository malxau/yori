
link: ypm.exe

install:
	@copy ypm.exe $(BINDIR) >NUL
	@if exist ypm.pdb copy ypm.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

CFLAGS=$(CFLAGS) -DYPM_VER_MAJOR=$(YPM_VER_MAJOR) -DYPM_VER_MINOR=$(YPM_VER_MINOR) -I..\pkglib

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdypm.pdb
LINKPDB=/Pdb:ypm.pdb
!ENDIF

OBJS=\
	 ypm.obj         \

compile: $(OBJS)

ypm.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib ..\pkglib\yoripkg.lib -version:$(YPM_VER_MAJOR).$(YPM_VER_MINOR) $(LINKPDB_YPM) -out:$@

