
BINARIES=ybase64.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ybase64.pdb

BIN_OBJS=\
	 base64.obj         \

MOD_OBJS=\
	 mod_base64.obj     \

compile: $(BIN_OBJS) builtins.lib

ybase64.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_base64.obj: base64.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ base64.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

