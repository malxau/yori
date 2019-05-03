
BINARIES=yshutdn.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdyshutdn.pdb
LINKPDB=/Pdb:yshutdn.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DSHUTDN_VER_MAJOR=$(SHUTDN_VER_MAJOR) -DSHUTDN_VER_MINOR=$(SHUTDN_VER_MINOR)

BIN_OBJS=\
	 shutdn.obj         \

MOD_OBJS=\
	 mod_shut.obj     \

compile: $(BIN_OBJS) builtins.lib

yshutdn.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(SHUTDN_VER_MAJOR).$(SHUTDN_VER_MINOR) $(LINKPDB) -out:$@

mod_shut.obj: shutdn.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ shutdn.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


