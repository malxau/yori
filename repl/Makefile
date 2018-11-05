
BINARIES=repl.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdrepl.pdb
LINKPDB=/Pdb:repl.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DREPL_VER_MAJOR=$(REPL_VER_MAJOR) -DREPL_VER_MINOR=$(REPL_VER_MINOR)

OBJS=\
	 repl.obj         \

compile: $(OBJS)

repl.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(REPL_VER_MAJOR).$(REPL_VER_MINOR) $(LINKPDB) -out:$@

