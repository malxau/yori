
BINARIES=sdir.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:sdir.pdb

BIN_OBJS=\
		 callback.obj  \
		 color.obj     \
		 display.obj   \
		 init.obj      \
		 sdir.obj      \
		 usage.obj     \
		 utils.obj

MOD_OBJS=\
		 callback.obj  \
		 color.obj     \
		 display.obj   \
		 init.obj      \
		 msdir.obj     \
		 usage.obj     \
		 utils.obj


compile: $(BIN_OBJS) builtins.lib

sdir.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

msdir.obj: sdir.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ sdir.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


