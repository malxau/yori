
BINARIES=petool.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:petool.pdb

BIN_OBJS=\
	 petool.obj         \

MOD_OBJS=\
	 mpetool.obj     \

compile: $(BIN_OBJS) builtins.lib

petool.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mpetool.obj: petool.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ petool.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

