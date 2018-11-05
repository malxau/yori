
BINARIES=ydate.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdydate.pdb
LINKPDB=/Pdb:ydate.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DDATE_VER_MAJOR=$(DATE_VER_MAJOR) -DDATE_VER_MINOR=$(DATE_VER_MINOR)

OBJS=\
	 date.obj        \

compile: $(OBJS)

ydate.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(DATE_VER_MAJOR).$(DATE_VER_MINOR) $(LINKPDB) -out:$@

