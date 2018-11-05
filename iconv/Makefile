
BINARIES=iconv.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdiconv.pdb
LINKPDB=/Pdb:iconv.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DICONV_VER_MAJOR=$(ICONV_VER_MAJOR) -DICONV_VER_MINOR=$(ICONV_VER_MINOR)

OBJS=\
	 iconv.obj         \

compile: $(OBJS)

iconv.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(ICONV_VER_MAJOR).$(ICONV_VER_MINOR) $(LINKPDB) -out:$@

