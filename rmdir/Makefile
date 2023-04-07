
BINARIES=yrmdir.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:yrmdir.pdb

BIN_OBJS=\
	 rmdir.obj        \

MOD_OBJS=\
	 mod_rmdir.obj    \

compile: $(BIN_OBJS) builtins.lib

yrmdir.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_rmdir.obj: rmdir.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ rmdir.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
