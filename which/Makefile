
BINARIES=which.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:which.pdb

BIN_OBJS=\
	 which.obj         \

MOD_OBJS=\
	 mwhich.obj     \

compile: $(BIN_OBJS) builtins.lib

which.exe: $(BIN_OBJS) $(YORILIBS)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mwhich.obj: which.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ which.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
