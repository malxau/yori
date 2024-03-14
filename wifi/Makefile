
BINARIES=ywifi.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ywifi.pdb

BIN_OBJS=\
	 wifi.obj         \

MOD_OBJS=\
	 mwifi.obj     \

compile: $(BIN_OBJS) builtins.lib

ywifi.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mwifi.obj: wifi.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ wifi.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
