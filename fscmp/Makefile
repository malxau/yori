
BINARIES=fscmp.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdfscmp.pdb
LINKPDB=/Pdb:fscmp.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DFSCMP_VER_MAJOR=$(FSCMP_VER_MAJOR) -DFSCMP_VER_MINOR=$(FSCMP_VER_MINOR)

OBJS=\
	 fscmp.obj        \

compile: $(OBJS)

fscmp.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(FSCMP_VER_MAJOR).$(FSCMP_VER_MINOR) $(LINKPDB) -out:$@

