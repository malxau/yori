
link: intcmp.exe

install:
	@copy intcmp.exe $(BINDIR) >NUL
	@if exist intcmp.pdb copy intcmp.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdintcmp.pdb
LINKPDB=/Pdb:intcmp.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DINTCMP_VER_MAJOR=$(INTCMP_VER_MAJOR) -DINTCMP_VER_MINOR=$(INTCMP_VER_MINOR)

OBJS=\
	 intcmp.obj        \

compile: $(OBJS)

intcmp.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(INTCMP_VER_MAJOR).$(INTCMP_VER_MINOR) $(LINKPDB) -out:$@

