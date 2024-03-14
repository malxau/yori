
BINARIES=fscmp.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:fscmp.pdb

BIN_OBJS=\
	 fscmp.obj        \

MOD_OBJS=\
	 mfscmp.obj    \

compile: $(BIN_OBJS) builtins.lib

fscmp.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mfscmp.obj: fscmp.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ fscmp.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
