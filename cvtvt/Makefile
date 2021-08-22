
BINARIES=cvtvt.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(CVTVT_VER_MAJOR)
YORI_BIN_VER_MINOR=$(CVTVT_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:cvtvt.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCVTVT_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DCVTVT_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 html.obj        \
	 main.obj        \
	 rtf.obj         \

MOD_OBJS=\
	 html.obj        \
	 mod_main.obj    \
	 rtf.obj         \

compile: $(BIN_OBJS) builtins.lib

cvtvt.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_main.obj: main.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ main.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
