
BINARIES=tee.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdtee.pdb
LINKPDB=/Pdb:tee.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DTEE_VER_MAJOR=$(TEE_VER_MAJOR) -DTEE_VER_MINOR=$(TEE_VER_MINOR)

OBJS=\
	 tee.obj         \

compile: $(OBJS)

tee.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(TEE_VER_MAJOR).$(TEE_VER_MINOR) $(LINKPDB) -out:$@

