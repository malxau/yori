
BINARIES=ypm.exe

!INCLUDE "..\config\common.mk"

CFLAGS=$(CFLAGS) -I..\pkglib

LINKPDB=/Pdb:ypm.pdb

BIN_OBJS=\
	 download.obj    \
	 list.obj        \
	 mirror.obj      \
	 rsource.obj     \
	 uninst.obj      \
	 upgrade.obj     \
	 ypm.obj         \
	 ypmconf.obj     \
	 ypmcreat.obj    \
	 ypminst.obj     \

MOD_OBJS=\
	 download.obj    \
	 list.obj        \
	 mirror.obj      \
	 rsource.obj     \
	 uninst.obj      \
	 upgrade.obj     \
	 mod_ypm.obj     \
	 ypmconf.obj     \
	 ypmcreat.obj    \
	 ypminst.obj     \

compile: $(BIN_OBJS) builtins.lib

ypm.exe: $(BIN_OBJS) $(YORILIBS) $(YORIPKG) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIPKG) $(FDILIB) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_ypm.obj: ypm.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ ypm.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

