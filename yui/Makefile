
BINARIES=yui.exe

!INCLUDE "..\config\common.mk"
YORI_BIN_VER_MAJOR=$(YORI_VER_MAJOR)
YORI_BIN_VER_MINOR=$(YORI_VER_MINOR)

!IF $(PDB)==1
LINKPDB=/Pdb:yui.pdb
!ENDIF

LDFLAGS_NT4UP=$(LDFLAGS:3.10=4.0)
EXTERNLIBS=$(EXTERNLIBS) user32.lib gdi32.lib comdlg32.lib

BIN_OBJS=\
	 menu.obj        \
	 resource.obj    \
	 taskbar.obj     \
	 yui.obj         \

compile: $(BIN_OBJS)

yui.exe: $(BIN_OBJS) $(YORILIBS)
	@echo $@
	@$(LINK) $(LDFLAGS_NT4UP) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

