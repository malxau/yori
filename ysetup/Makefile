
BINARIES=ysetup.exe

!INCLUDE "..\config\common.mk"
YORI_BIN_VER_MAJOR=$(YORI_VER_MAJOR)
YORI_BIN_VER_MINOR=$(YORI_VER_MINOR)
YSETUP_TUI=1

CFLAGS=$(CFLAGS) -I..\pkglib -DYSETUP_TUI=$(YSETUP_TUI)

LINKPDB=/Pdb:ysetup.pdb

OBJS=\
	 resource.obj    \
	 gui.obj         \
	 helper.obj      \
	 tui.obj         \
	 ysetup.obj      \

compile: $(OBJS)

ysetup.exe: $(OBJS) $(YORILIBS) $(YORIPKG) $(YORIWIN) $(YORIDLG)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIPKG) $(FDILIB) $(YORIWIN) $(YORIDLG) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

