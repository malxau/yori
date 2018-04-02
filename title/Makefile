
link: ytitle.exe

install:
	@copy ytitle.exe $(BINDIR) >NUL
	@if exist ytitle.pdb copy ytitle.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdytitle.pdb
LINKPDB=/Pdb:ytitle.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DTITLE_VER_MAJOR=$(TITLE_VER_MAJOR) -DTITLE_VER_MINOR=$(TITLE_VER_MINOR)

OBJS=\
	 title.obj         \

compile: $(OBJS)

ytitle.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(TITLE_VER_MAJOR).$(TITLE_VER_MINOR) $(LINKPDB) -out:$@

