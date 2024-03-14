
BINARIES=ydu.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ydu.pdb

BIN_OBJS=\
	 du.obj         \

MOD_OBJS=\
	 mdu.obj     \

compile: $(BIN_OBJS) builtins.lib

ydu.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mdu.obj: du.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ du.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


