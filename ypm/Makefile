
link: ypm.exe

install:
	@copy ypm.exe $(BINDIR) >NUL
	@if exist ypm.pdb copy ypm.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdypm.pdb
LINKPDB=/Pdb:ypm.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DYPM_VER_MAJOR=$(YPM_VER_MAJOR) -DYPM_VER_MINOR=$(YPM_VER_MINOR)

OBJS=\
	 ypm.obj         \

compile: $(OBJS)

ypm.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(YPM_VER_MAJOR).$(YPM_VER_MINOR) $(LINKPDB) -out:$@

