
link: scut.exe

install:
	@copy scut.exe $(BINDIR) >NUL
	@if exist scut.pdb copy scut.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdscut.pdb
LINKPDB=/Pdb:scut.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DSCUT_VER_MAJOR=$(SCUT_VER_MAJOR) -DSCUT_VER_MINOR=$(SCUT_VER_MINOR)

OBJS=\
	 scut.obj         \

!IFDEF _NMAKE_VER
.c.obj::
!ELSE
.c.obj:
!ENDIF
	@$(CC) $(CFLAGS) -c $<

compile: $(OBJS)

scut.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib uuid.lib shell32.lib ole32.lib -version:$(SCUT_VER_MAJOR).$(SCUT_VER_MINOR) $(LINKPDB) -out:$@

