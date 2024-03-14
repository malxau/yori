
BINARIES=wininfo.exe

!INCLUDE "..\config\common.mk"
YORI_BIN_VER_MAJOR=$(YORI_VER_MAJOR)
YORI_BIN_VER_MINOR=$(YORI_VER_MINOR)

LINKPDB=/Pdb:wininfo.pdb

BIN_OBJS=\
	 wininfo.obj         \
	 resource.obj        \

MOD_OBJS=\
	 mwininfo.obj     \

compile: $(BIN_OBJS) builtins.lib

wininfo.exe: $(BIN_OBJS) $(YORILIBS)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mwininfo.obj: wininfo.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ wininfo.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
