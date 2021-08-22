
BINARIES=ypm.exe

!INCLUDE "..\config\ver.mk"
YORI_BIN_VER_MAJOR=$(YPM_VER_MAJOR)
YORI_BIN_VER_MINOR=$(YPM_VER_MINOR)
!INCLUDE "..\config\common.mk"

CFLAGS=$(CFLAGS) -DYPM_VER_MAJOR=$(YORI_BIN_VER_MAJOR) -DYPM_VER_MINOR=$(YORI_BIN_VER_MINOR) -I..\pkglib

!IF $(PDB)==1
LINKPDB=/Pdb:ypm.pdb
!ENDIF

BIN_OBJS=\
	 ypm.obj         \

MOD_OBJS=\
	 mod_ypm.obj     \

compile: $(BIN_OBJS) builtins.lib

ypm.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT) $(YORIPKG) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) $(YORIPKG) $(FDILIB) $(YORIVER) -version:$(YORI_BIN_VER_MAJOR).$(YORI_BIN_VER_MINOR) $(LINKPDB) -out:$@

mod_ypm.obj: ypm.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ ypm.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

