
BINARIES=ydate.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(DATE_VER_MAJOR)
YORI_BIN_VER_MINOR=$(DATE_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ydate.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DDATE_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DDATE_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 date.obj        \

MOD_OBJS=\
	 mod_date.obj    \

compile: $(BIN_OBJS) builtins.lib

ydate.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_date.obj: date.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ date.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

