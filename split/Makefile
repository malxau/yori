
BINARIES=ysplit.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ysplit.pdb

BIN_OBJS=\
	 split.obj         \

MOD_OBJS=\
	 msplit.obj     \

compile: $(BIN_OBJS) builtins.lib

ysplit.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

msplit.obj: split.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ split.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

