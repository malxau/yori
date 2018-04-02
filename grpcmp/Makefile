
link: grpcmp.exe

install:
	@copy grpcmp.exe $(BINDIR) >NUL
	@if exist grpcmp.pdb copy grpcmp.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdgrpcmp.pdb
LINKPDB=/Pdb:grpcmp.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DGRPCMP_VER_MAJOR=$(GRPCMP_VER_MAJOR) -DGRPCMP_VER_MINOR=$(GRPCMP_VER_MINOR)

OBJS=\
	 grpcmp.obj        \

compile: $(OBJS)

grpcmp.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib advapi32.lib -version:$(GRPCMP_VER_MAJOR).$(GRPCMP_VER_MINOR) $(LINKPDB) -out:$@

