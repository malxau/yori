
BINARIES=yexpr.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdyexpr.pdb
LINKPDB=/Pdb:yexpr.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DEXPR_VER_MAJOR=$(EXPR_VER_MAJOR) -DEXPR_VER_MINOR=$(EXPR_VER_MINOR)

OBJS=\
	 expr.obj         \

compile: $(OBJS)

yexpr.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(EXPR_VER_MAJOR).$(EXPR_VER_MINOR) $(LINKPDB) -out:$@

