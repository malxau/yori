
BINARIES=ycal.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(CAL_VER_MAJOR)
YORI_BIN_VER_MINOR=$(CAL_VER_MINOR)
!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ycal.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCAL_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DCAL_VER_MINOR=$(YORI_BIN_VER_MINOR)

BIN_OBJS=\
	 cal.obj         \

MOD_OBJS=\
	 mod_cal.obj     \

compile: $(BIN_OBJS) builtins.lib

ycal.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_cal.obj: cal.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ cal.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


