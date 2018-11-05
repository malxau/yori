
BINARIES=ycopy.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdycopy.pdb
LINKPDB=/Pdb:ycopy.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCOPY_VER_MAJOR=$(COPY_VER_MAJOR) -DCOPY_VER_MINOR=$(COPY_VER_MINOR)

OBJS=\
	 copy.obj         \

compile: $(OBJS)

ycopy.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(COPY_VER_MAJOR).$(COPY_VER_MINOR) $(LINKPDB) -out:$@

