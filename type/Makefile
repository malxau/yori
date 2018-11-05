
BINARIES=ytype.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdytype.pdb
LINKPDB=/Pdb:ytype.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DTYPE_VER_MAJOR=$(TYPE_VER_MAJOR) -DTYPE_VER_MINOR=$(TYPE_VER_MINOR)

OBJS=\
	 type.obj         \

compile: $(OBJS)

ytype.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(TYPE_VER_MAJOR).$(TYPE_VER_MINOR) $(LINKPDB) -out:$@

