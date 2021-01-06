
BINARIES=yattrib.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:yattrib.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DATTRIB_VER_MAJOR=$(ATTRIB_VER_MAJOR) -DATTRIB_VER_MINOR=$(ATTRIB_VER_MINOR)

BIN_OBJS=\
	 attrib.obj         \

MOD_OBJS=\
	 mod_attrib.obj     \

compile: $(BIN_OBJS) builtins.lib

yattrib.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) -version:$(ATTRIB_VER_MAJOR).$(ATTRIB_VER_MINOR) $(LINKPDB) -out:$@

mod_attrib.obj: attrib.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ attrib.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

