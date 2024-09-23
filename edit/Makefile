
BINARIES=yedit.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:yedit.pdb

BIN_OBJS=\
	 edit.obj         \
	 options.obj      \
	 resource.obj     \

MOD_OBJS=\
	 medit.obj        \
	 options.obj      \

compile: $(BIN_OBJS) builtins.lib

yedit.exe: $(BIN_OBJS) $(YORILIBS) $(YORIWIN) $(YORIDLG)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIWIN) $(YORIDLG) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

medit.obj: edit.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ edit.c

resource.obj: resource.rc en-us.rc resource.h
	@echo resource.rc
	@if exist $@ erase $@
	@$(RC) /fo$(@B).res $(RCFLAGS) /d YORI_VER_MAJOR=$(YORI_BASE_VER_MAJOR) /d YORI_VER_MINOR=$(YORI_BASE_VER_MINOR) /d YORI_BIN_VER_MAJOR=$(YORI_BASE_VER_MAJOR) /d YORI_BIN_VER_MINOR=$(YORI_BASE_VER_MINOR) resource.rc >NUL
	@if not exist $@ ren $(@B).res $@

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
