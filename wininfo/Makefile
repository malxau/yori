
BINARIES=wininfo.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdwininfo.pdb
LINKPDB=/Pdb:wininfo.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DWININFO_VER_MAJOR=$(WININFO_VER_MAJOR) -DWININFO_VER_MINOR=$(WININFO_VER_MINOR)

OBJS=\
	 wininfo.obj         \

compile: $(OBJS)

wininfo.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(WININFO_VER_MAJOR).$(WININFO_VER_MINOR) $(LINKPDB) -out:$@

