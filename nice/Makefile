
BINARIES=nice.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(NICE_VER_MAJOR)
YORI_BIN_VER_MINOR=$(NICE_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:nice.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DNICE_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DNICE_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 nice.obj         \

MOD_OBJS=\
	 mod_nice.obj     \

compile: $(BIN_OBJS) builtins.lib

nice.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_nice.obj: nice.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ nice.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

