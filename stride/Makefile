
BINARIES=ystride.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ystride.pdb

BIN_OBJS=\
	 stride.obj         \

MOD_OBJS=\
	 mod_stride.obj     \

compile: $(BIN_OBJS) builtins.lib

ystride.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_stride.obj: stride.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ stride.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
