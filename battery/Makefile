
BINARIES=ybattery.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ybattery.pdb

BIN_OBJS=\
	 battery.obj         \

MOD_OBJS=\
	 mod_battery.obj     \

compile: $(BIN_OBJS) builtins.lib

ybattery.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_battery.obj: battery.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ battery.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
