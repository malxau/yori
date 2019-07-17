
BINARIES=ysplit.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ysplit.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DSPLIT_VER_MAJOR=$(SPLIT_VER_MAJOR) -DSPLIT_VER_MINOR=$(SPLIT_VER_MINOR)

BIN_OBJS=\
	 split.obj         \

MOD_OBJS=\
	 mod_split.obj     \

compile: $(BIN_OBJS) builtins.lib

ysplit.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(SPLIT_VER_MAJOR).$(SPLIT_VER_MINOR) $(LINKPDB) -out:$@

mod_split.obj: split.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ split.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

