
BINARIES=wininfo.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:wininfo.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DWININFO_VER_MAJOR=$(WININFO_VER_MAJOR) -DWININFO_VER_MINOR=$(WININFO_VER_MINOR)

BIN_OBJS=\
	 wininfo.obj         \

MOD_OBJS=\
	 mod_wininfo.obj     \

compile: $(BIN_OBJS) builtins.lib

wininfo.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(WININFO_VER_MAJOR).$(WININFO_VER_MINOR) $(LINKPDB) -out:$@

mod_wininfo.obj: wininfo.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ wininfo.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
