
BINARIES=ydf.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ydf.pdb

BIN_OBJS=\
	 df.obj         \

MOD_OBJS=\
	 mdf.obj     \

compile: $(BIN_OBJS) builtins.lib

ydf.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mdf.obj: df.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ df.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


