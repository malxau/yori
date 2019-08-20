
BINARIES=ysetup.exe

!INCLUDE "..\config\common.mk"

CFLAGS=$(CFLAGS) -DYSETUP_VER_MAJOR=$(YSETUP_VER_MAJOR) -DYSETUP_VER_MINOR=$(YSETUP_VER_MINOR) -I..\pkglib

!IF $(PDB)==1
LINKPDB=/Pdb:ysetup.pdb
!ENDIF

# This currently supports NT 3.51 and up.  For 3.51 it depends on a manually
# installed WinInet.dll and Cabinet.dll.  But it's okay for this program to
# import functions on 3.51+ because it has no real chance to run on anything
# older.
LIBS=$(LIBS) user32.lib gdi32.lib advapi32.lib
LDFLAGS_NT4UP=$(LDFLAGS:3.10=4.0)

OBJS=\
	 resource.obj    \
	 ysetup.obj      \

compile: $(OBJS)

ysetup.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS_NT4UP) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib ..\pkglib\yoripkg.lib -version:$(YSETUP_VER_MAJOR).$(YSETUP_VER_MINOR) $(LINKPDB_YSETUP) -out:$@

