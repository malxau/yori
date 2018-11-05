
MODULES=\
	for.com

BINARIES=yfor.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdyfor.pdb
LINKPDB=/Pdb:yfor.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DFOR_VER_MAJOR=$(FOR_VER_MAJOR) -DFOR_VER_MINOR=$(FOR_VER_MINOR)

BIN_OBJS=\
	for.obj        \

MOD_OBJS=\
	mod_for.obj    \

compile: $(BIN_OBJS) $(MOD_OBJS) builtins.lib

yfor.exe: $(BIN_OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(FOR_VER_MAJOR).$(FOR_VER_MINOR) $(LINKPDB) -out:$@

mod_for.obj: for.c
	@echo $@
	@cl -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ for.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

for.com: $(MOD_OBJS)
	@echo $@
	@$(LINK) $(LDFLAGS) $(MOD_OBJS) -dll -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -def:for.def -out:$@
