
BINARIES=yecho.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:yecho.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DECHO_VER_MAJOR=$(ECHO_VER_MAJOR) -DECHO_VER_MINOR=$(ECHO_VER_MINOR)

BIN_OBJS=\
	 echo.obj         \

MOD_OBJS=\
	 mod_echo.obj     \

compile: $(BIN_OBJS) builtins.lib

yecho.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(ECHO_VER_MAJOR).$(ECHO_VER_MINOR) $(LINKPDB) -out:$@

mod_echo.obj: echo.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ echo.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


