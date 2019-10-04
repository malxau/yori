
BINARIES=ystride.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ystride.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DSTRIDE_VER_MAJOR=$(STRIDE_VER_MAJOR) -DSTRIDE_VER_MINOR=$(STRIDE_VER_MINOR)

BIN_OBJS=\
	 stride.obj         \

MOD_OBJS=\
	 mod_stride.obj     \

compile: $(BIN_OBJS) builtins.lib

ystride.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(STRIDE_VER_MAJOR).$(STRIDE_VER_MINOR) $(LINKPDB) -out:$@

mod_stride.obj: stride.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ stride.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
