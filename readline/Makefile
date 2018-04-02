
link: readline.exe

install:
	@copy readline.exe $(BINDIR) >NUL
	@if exist readline.pdb copy readline.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdreadline.pdb
LINKPDB=/Pdb:readline.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DREADLINE_VER_MAJOR=$(READLINE_VER_MAJOR) -DREADLINE_VER_MINOR=$(READLINE_VER_MINOR)

OBJS=\
	 readline.obj         \

compile: $(OBJS)

readline.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(READLINE_VER_MAJOR).$(READLINE_VER_MINOR) $(LINKPDB) -out:$@

