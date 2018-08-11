
link: ypm.exe ysetup.exe

install:
	@copy ypm.exe $(BINDIR) >NUL
	@if exist ypm.pdb copy ypm.pdb $(SYMDIR) >NUL
	@copy ysetup.exe $(BINDIR) >NUL
	@if exist ysetup.pdb copy ysetup.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

CFLAGS=$(CFLAGS) -DYPM_VER_MAJOR=$(YPM_VER_MAJOR) -DYPM_VER_MINOR=$(YPM_VER_MINOR)

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdypm.pdb
LINKPDB_YPM=/Pdb:ypm.pdb
LINKPDB_YSETUP=/Pdb:ysetup.pdb
!ENDIF

LIBOBJS=\
	 create.obj      \
	 install.obj     \
	 remote.obj      \
	 util.obj        \

ALLOBJS=\
	 $(LIBOBJS)      \
	 ypm.obj         \
	 ysetup.obj      \

compile: $(ALLOBJS)

ypm.exe: $(ALLOBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(LIBOBJS) ypm.obj $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(YPM_VER_MAJOR).$(YPM_VER_MINOR) $(LINKPDB_YPM) -out:$@

ysetup.exe: $(ALLOBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(LIBOBJS) ysetup.obj $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(YPM_VER_MAJOR).$(YPM_VER_MINOR) $(LINKPDB_YSETUP) -out:$@

