
BINARIES=touch.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(TOUCH_VER_MAJOR)
YORI_BIN_VER_MINOR=$(TOUCH_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:touch.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DTOUCH_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DTOUCH_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 touch.obj         \

MOD_OBJS=\
	 mod_touch.obj     \

compile: $(BIN_OBJS) builtins.lib

touch.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_touch.obj: touch.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ touch.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
