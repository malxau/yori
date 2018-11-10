
BINARIES=iconv.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdiconv.pdb
LINKPDB=/Pdb:iconv.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DICONV_VER_MAJOR=$(ICONV_VER_MAJOR) -DICONV_VER_MINOR=$(ICONV_VER_MINOR)

BIN_OBJS=\
	 iconv.obj         \

MOD_OBJS=\
	 mod_iconv.obj     \

compile: $(BIN_OBJS) builtins.lib

iconv.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(ICONV_VER_MAJOR).$(ICONV_VER_MINOR) $(LINKPDB) -out:$@

mod_iconv.obj: iconv.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ iconv.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
