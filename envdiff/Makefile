
BINARIES=envdiff.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:envdiff.pdb

BIN_OBJS=\
	 envdiff.obj        \

MOD_OBJS=\
	 menvdiff.obj    \

compile: $(BIN_OBJS) builtins.lib

envdiff.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

menvdiff.obj: envdiff.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ envdiff.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

