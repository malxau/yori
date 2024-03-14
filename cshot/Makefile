
BINARIES=cshot.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:cshot.pdb

BIN_OBJS=\
	 cshot.obj        \

MOD_OBJS=\
	 mcshot.obj    \

compile: $(BIN_OBJS) builtins.lib

cshot.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mcshot.obj: cshot.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ cshot.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

