
BINARIES=ysetup.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(YSETUP_VER_MAJOR)
YORI_BIN_VER_MINOR=$(YSETUP_VER_MINOR)
!INCLUDE "..\config\common.mk"

CFLAGS=$(CFLAGS) -DYSETUP_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DYSETUP_VER_MINOR=$(YORI_BIN_VER_MINOR) -I..\pkglib

!IF $(PDB)==1
LINKPDB=/Pdb:ysetup.pdb
!ENDIF

# This currently supports NT 3.5 and up.  For 3.5x it depends on a manually
# installed WinInet.dll and Cabinet.dll.  Note that 3.5x will launch anything
# stamped as version 4.0, and stamping it as 4.0 gives a nicer UI on newer
# systems.
LIBS=$(LIBS) gdi32.lib user32.lib
LDFLAGS_NT4UP=$(LDFLAGS:3.10=4.0)

OBJS=\
	 resource.obj    \
	 ysetup.obj      \

compile: $(OBJS)

ysetup.exe: $(OBJS) $(YORILIB) $(YORICRT) $(YORIPKG)
	@echo $@
	@$(LINK) $(LDFLAGS_NT4UP) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIPKG) $(FDILIB) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

