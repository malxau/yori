
link: hexdump.exe

install:
	@copy hexdump.exe $(BINDIR) >NUL
	@if exist hexdump.pdb copy hexdump.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdhexdump.pdb
LINKPDB=/Pdb:hexdump.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DHEXDUMP_VER_MAJOR=$(HEXDUMP_VER_MAJOR) -DHEXDUMP_VER_MINOR=$(HEXDUMP_VER_MINOR)

OBJS=\
	 hexdump.obj         \

compile: $(OBJS)

hexdump.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(HEXDUMP_VER_MAJOR).$(HEXDUMP_VER_MINOR) $(LINKPDB) -out:$@

