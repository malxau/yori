
MODULES=\
	for.com

BINARIES=yfor.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:yfor.pdb

BIN_OBJS=\
	for.obj        \

MOD_OBJS=\
	mfor.obj    \

compile: $(BIN_OBJS) builtins.lib

yfor.exe: $(BIN_OBJS) $(YORILIBS) $(YORISH) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORISH) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mfor.obj: for.c
	@echo $@
	@cl -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ for.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

for.com: $(MOD_OBJS) $(YORILIBS) $(YORISH) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) $(MOD_OBJS) -dll $(YORILIBS) $(EXTERNLIBS) $(YORISH) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) -def:for.def -out:$@
