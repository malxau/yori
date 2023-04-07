
BINARIES=wintest.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:wintest.pdb

BIN_OBJS=\
	 wintest.obj         \

compile: $(BIN_OBJS)

wintest.exe: $(BIN_OBJS) $(YORILIBS) $(YORIWIN) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIWIN) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@
