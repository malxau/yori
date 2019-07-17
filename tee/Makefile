
BINARIES=tee.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:tee.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DTEE_VER_MAJOR=$(TEE_VER_MAJOR) -DTEE_VER_MINOR=$(TEE_VER_MINOR)

BIN_OBJS=\
	 tee.obj         \

MOD_OBJS=\
	 mod_tee.obj     \

compile: $(BIN_OBJS) builtins.lib

tee.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(TEE_VER_MAJOR).$(TEE_VER_MINOR) $(LINKPDB) -out:$@

mod_tee.obj: tee.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ tee.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
