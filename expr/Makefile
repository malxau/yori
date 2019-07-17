
BINARIES=yexpr.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:yexpr.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DEXPR_VER_MAJOR=$(EXPR_VER_MAJOR) -DEXPR_VER_MINOR=$(EXPR_VER_MINOR)

BIN_OBJS=\
	 expr.obj         \

MOD_OBJS=\
	 mod_expr.obj     \

compile: $(BIN_OBJS) builtins.lib

yexpr.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(EXPR_VER_MAJOR).$(EXPR_VER_MINOR) $(LINKPDB) -out:$@

mod_expr.obj: expr.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ expr.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


