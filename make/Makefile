
BINARIES=ymake.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ymake.pdb

BIN_OBJS=\
	 alloc.obj        \
	 exec.obj         \
	 make.obj         \
	 minish.obj       \
	 preproc.obj      \
	 scope.obj        \
	 target.obj       \
	 var.obj          \

MOD_OBJS=\
	 alloc.obj        \
	 exec.obj         \
	 mod_make.obj     \
	 minish.obj       \
	 preproc.obj      \
	 scope.obj        \
	 target.obj       \
	 var.obj          \

compile: $(BIN_OBJS) builtins.lib

MAKE_LIBS=$(YORILIBS) $(YORISH) $(YORIVER) ..\builtins\builtins.lib ..\echo\builtins.lib ..\mkdir\builtins.lib ..\rmdir\builtins.lib

ymake.exe: $(BIN_OBJS) $(MAKE_LIBS)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(MAKE_LIBS) $(EXTERNLIBS) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_make.obj: make.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ make.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
