
BINARIES=clmp.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:clmp.pdb

OBJS=\
	 clmp.obj        \

compile: $(OBJS)

clmp.exe: $(OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

