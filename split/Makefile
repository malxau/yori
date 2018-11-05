
BINARIES=ysplit.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdysplit.pdb
LINKPDB=/Pdb:ysplit.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DSPLIT_VER_MAJOR=$(SPLIT_VER_MAJOR) -DSPLIT_VER_MINOR=$(SPLIT_VER_MINOR)

OBJS=\
	 split.obj         \

compile: $(OBJS)

ysplit.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(SPLIT_VER_MAJOR).$(SPLIT_VER_MINOR) $(LINKPDB) -out:$@

