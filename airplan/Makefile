
BINARIES=yairplan.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:yairplan.pdb

BIN_OBJS=\
	 airplan.obj         \

MOD_OBJS=\
	 mairplan.obj     \

compile: $(BIN_OBJS) builtins.lib

yairplan.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mairplan.obj: airplan.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ airplan.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
