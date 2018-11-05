
BINARIES=touch.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdtouch.pdb
LINKPDB=/Pdb:touch.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DTOUCH_VER_MAJOR=$(TOUCH_VER_MAJOR) -DTOUCH_VER_MINOR=$(TOUCH_VER_MINOR)

OBJS=\
	 touch.obj         \

compile: $(OBJS)

touch.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(TOUCH_VER_MAJOR).$(TOUCH_VER_MINOR) $(LINKPDB) -out:$@

