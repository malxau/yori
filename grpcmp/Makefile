
BINARIES=grpcmp.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:grpcmp.pdb

BIN_OBJS=\
	 grpcmp.obj        \

MOD_OBJS=\
	 mod_grpcmp.obj    \

compile: $(BIN_OBJS) builtins.lib

grpcmp.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_grpcmp.obj: grpcmp.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ grpcmp.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
