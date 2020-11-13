
BINARIES=ydu.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ydu.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DDU_VER_MAJOR=$(DU_VER_MAJOR) -DDU_VER_MINOR=$(DU_VER_MINOR)

BIN_OBJS=\
	 du.obj         \

MOD_OBJS=\
	 mod_du.obj     \

compile: $(BIN_OBJS) builtins.lib

ydu.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) -version:$(DU_VER_MAJOR).$(DU_VER_MINOR) $(LINKPDB) -out:$@

mod_du.obj: du.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ du.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


