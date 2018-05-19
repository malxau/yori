
link: yclip.exe

install:
	@copy yclip.exe $(BINDIR) >NUL
	@if exist yclip.pdb copy yclip.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdyclip.pdb
LINKPDB=/Pdb:yclip.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCLIP_VER_MAJOR=$(CLIP_VER_MAJOR) -DCLIP_VER_MINOR=$(CLIP_VER_MINOR)

OBJS=\
	 clip.obj        \

compile: $(OBJS)

yclip.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(CLIP_VER_MAJOR).$(CLIP_VER_MINOR) $(LINKPDB) -out:$@

