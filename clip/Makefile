
BINARIES=yclip.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:yclip.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCLIP_VER_MAJOR=$(CLIP_VER_MAJOR) -DCLIP_VER_MINOR=$(CLIP_VER_MINOR)

BIN_OBJS=\
	 clip.obj        \

MOD_OBJS=\
	 mod_clip.obj    \

compile: $(BIN_OBJS) builtins.lib

yclip.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(CLIP_VER_MAJOR).$(CLIP_VER_MINOR) $(LINKPDB) -out:$@

mod_clip.obj: clip.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ clip.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

