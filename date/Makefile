
BINARIES=ydate.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ydate.pdb

BIN_OBJS=\
	 date.obj        \

MOD_OBJS=\
	 mod_date.obj    \

compile: $(BIN_OBJS) builtins.lib

ydate.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mod_date.obj: date.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ date.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@

