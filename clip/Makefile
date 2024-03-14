
BINARIES=yclip.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:yclip.pdb

BIN_OBJS=\
	 clip.obj        \

MOD_OBJS=\
	 mclip.obj    \

compile: $(BIN_OBJS) builtins.lib

yclip.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mclip.obj: clip.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ clip.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

