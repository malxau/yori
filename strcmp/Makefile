
BINARIES=ystrcmp.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ystrcmp.pdb

BIN_OBJS=\
	 strcmp.obj        \

MOD_OBJS=\
	 mod_strcmp.obj    \

compile: $(BIN_OBJS) builtins.lib

ystrcmp.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_strcmp.obj: strcmp.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ strcmp.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
