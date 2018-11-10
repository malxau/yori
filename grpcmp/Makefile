
BINARIES=grpcmp.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdgrpcmp.pdb
LINKPDB=/Pdb:grpcmp.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DGRPCMP_VER_MAJOR=$(GRPCMP_VER_MAJOR) -DGRPCMP_VER_MINOR=$(GRPCMP_VER_MINOR)

BIN_OBJS=\
	 grpcmp.obj        \

MOD_OBJS=\
	 mod_grpcmp.obj    \

compile: $(BIN_OBJS) builtins.lib

grpcmp.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(GRPCMP_VER_MAJOR).$(GRPCMP_VER_MINOR) $(LINKPDB) -out:$@

mod_grpcmp.obj: grpcmp.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ grpcmp.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
