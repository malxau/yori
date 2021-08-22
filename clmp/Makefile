
BINARIES=clmp.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(CLMP_VER_MAJOR)
YORI_BIN_VER_MINOR=$(CLMP_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:clmp.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCLMP_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DCLMP_VER_MINOR=$(YORI_BIN_VER_MINOR)

OBJS=\
	 clmp.obj        \

compile: $(OBJS)

clmp.exe: $(OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

