
BINARIES=ymklink.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdymklink.pdb
LINKPDB=/Pdb:ymklink.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DMKLINK_VER_MAJOR=$(MKLINK_VER_MAJOR) -DMKLINK_VER_MINOR=$(MKLINK_VER_MINOR)

OBJS=\
	 mklink.obj         \

compile: $(OBJS)

ymklink.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(MKLINK_VER_MAJOR).$(MKLINK_VER_MINOR) $(LINKPDB) -out:$@

