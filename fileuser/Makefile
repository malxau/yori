
link: fileuser.exe

install:
	@copy fileuser.exe $(BINDIR) >NUL
	@if exist fileuser.pdb copy fileuser.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdfileuser.pdb
LINKPDB=/Pdb:fileuser.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DFILEUSER_VER_MAJOR=$(FILEUSER_VER_MAJOR) -DFILEUSER_VER_MINOR=$(FILEUSER_VER_MINOR)

OBJS=\
	 fileuser.obj         \

compile: $(OBJS)

fileuser.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(FILEUSER_VER_MAJOR).$(FILEUSER_VER_MINOR) $(LINKPDB) -out:$@

