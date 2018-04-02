
link: strcmp.exe

install:
	@copy strcmp.exe $(BINDIR) >NUL
	@if exist strcmp.pdb copy strcmp.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdstrcmp.pdb
LINKPDB=/Pdb:strcmp.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DSTRCMP_VER_MAJOR=$(STRCMP_VER_MAJOR) -DSTRCMP_VER_MINOR=$(STRCMP_VER_MINOR)

OBJS=\
	 strcmp.obj        \

compile: $(OBJS)

strcmp.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(STRCMP_VER_MAJOR).$(STRCMP_VER_MINOR) $(LINKPDB) -out:$@

