
BINARIES=ytitle.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(TITLE_VER_MAJOR)
YORI_BIN_VER_MINOR=$(TITLE_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ytitle.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DTITLE_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DTITLE_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 title.obj         \

MOD_OBJS=\
	 mod_title.obj     \

compile: $(BIN_OBJS) builtins.lib

ytitle.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_title.obj: title.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ title.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
