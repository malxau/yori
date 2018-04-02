
link: yerase.exe

install:
	@copy yerase.exe $(BINDIR) >NUL
	@if exist yerase.pdb copy yerase.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdyerase.pdb
LINKPDB=/Pdb:yerase.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DERASE_VER_MAJOR=$(ERASE_VER_MAJOR) -DERASE_VER_MINOR=$(ERASE_VER_MINOR)

OBJS=\
	 erase.obj        \

compile: $(OBJS)

yerase.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(ERASE_VER_MAJOR).$(ERASE_VER_MINOR) $(LINKPDB) -out:$@

