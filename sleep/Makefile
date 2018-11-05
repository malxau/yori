
BINARIES=sleep.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdsleep.pdb
LINKPDB=/Pdb:sleep.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DSLEEP_VER_MAJOR=$(SLEEP_VER_MAJOR) -DSLEEP_VER_MINOR=$(SLEEP_VER_MINOR)

OBJS=\
	 sleep.obj        \

compile: $(OBJS)

sleep.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(SLEEP_VER_MAJOR).$(SLEEP_VER_MINOR) $(LINKPDB) -out:$@

