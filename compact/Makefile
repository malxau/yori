
BINARIES=ycompact.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdycompact.pdb
LINKPDB=/Pdb:ycompact.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCOMPACT_VER_MAJOR=$(COMPACT_VER_MAJOR) -DCOMPACT_VER_MINOR=$(COMPACT_VER_MINOR)

OBJS=\
	 compact.obj         \

compile: $(OBJS)

ycompact.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(COMPACT_VER_MAJOR).$(COMPACT_VER_MINOR) $(LINKPDB) -out:$@

