
BINARIES=ydf.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ydf.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DDF_VER_MAJOR=$(DF_VER_MAJOR) -DDF_VER_MINOR=$(DF_VER_MINOR)

BIN_OBJS=\
	 df.obj         \

MOD_OBJS=\
	 mod_df.obj     \

compile: $(BIN_OBJS) builtins.lib

ydf.exe: $(BIN_OBJS) $(YORILIB) $(YORICRT)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) $(YORILIB) -version:$(DF_VER_MAJOR).$(DF_VER_MINOR) $(LINKPDB) -out:$@

mod_df.obj: df.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ df.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


