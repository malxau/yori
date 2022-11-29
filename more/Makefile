
BINARIES=ymore.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ymore.pdb
!ENDIF

BIN_OBJS=\
	 ingest.obj       \
	 moreinit.obj     \
	 more.obj         \
	 viewport.obj     \

MOD_OBJS=\
	 ingest.obj       \
	 moreinit.obj     \
	 mod_more.obj     \
	 viewport.obj     \

compile: $(BIN_OBJS) builtins.lib

ymore.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_more.obj: more.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ more.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
