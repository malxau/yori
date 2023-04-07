
BINARIES=yoritest.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:yoritest.pdb

BIN_OBJS=\
	 test.obj         \
	 argcargv.obj     \
	 fileenum.obj     \
	 parse.obj        \

compile: $(BIN_OBJS)

yoritest.exe: $(BIN_OBJS) $(YORILIBS) $(YORISH) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORISH) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@
