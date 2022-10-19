
BINARIES=yoritest.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:yoritest.pdb
!ENDIF

BIN_OBJS=\
	 test.obj         \
	 argcargv.obj     \
	 fileenum.obj     \
	 parse.obj        \

compile: $(BIN_OBJS)

yoritest.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORISH) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORISH) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@
