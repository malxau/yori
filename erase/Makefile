
BINARIES=yerase.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(ERASE_VER_MAJOR)
YORI_BIN_VER_MINOR=$(ERASE_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:yerase.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DERASE_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DERASE_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 erase.obj        \

MOD_OBJS=\
	 mod_erase.obj    \

compile: $(BIN_OBJS) builtins.lib

yerase.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_erase.obj: erase.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ erase.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
