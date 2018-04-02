
link: yrmdir.exe

install:
	@copy yrmdir.exe $(BINDIR) >NUL
	@if exist yrmdir.pdb copy yrmdir.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdyrmdir.pdb
LINKPDB=/Pdb:yrmdir.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DRMDIR_VER_MAJOR=$(RMDIR_VER_MAJOR) -DRMDIR_VER_MINOR=$(RMDIR_VER_MINOR)

OBJS=\
	 rmdir.obj        \

compile: $(OBJS)

yrmdir.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(RMDIR_VER_MAJOR).$(RMDIR_VER_MINOR) $(LINKPDB) -out:$@

