
BINARIES=cvtvt.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:cvtvt.pdb

BIN_OBJS=\
	 html.obj        \
	 main.obj        \
	 rtf.obj         \

MOD_OBJS=\
	 html.obj        \
	 mmain.obj    \
	 rtf.obj         \

compile: $(BIN_OBJS) builtins.lib

cvtvt.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mmain.obj: main.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ main.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
