
link: yhelp.exe

install:
	@copy yhelp.exe $(BINDIR) >NUL
	@if exist yhelp.pdb copy yhelp.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdyhelp.pdb
LINKPDB=/Pdb:yhelp.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DHELP_VER_MAJOR=$(HELP_VER_MAJOR) -DHELP_VER_MINOR=$(HELP_VER_MINOR)

OBJS=\
	 help.obj         \

compile: $(OBJS)

yhelp.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(HELP_VER_MAJOR).$(HELP_VER_MINOR) $(LINKPDB) -out:$@

