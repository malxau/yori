
BINARIES=ycal.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:ycal.pdb

BIN_OBJS=\
	 cal.obj         \

MOD_OBJS=\
	 mcal.obj     \

compile: $(BIN_OBJS) builtins.lib

ycal.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mcal.obj: cal.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ cal.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@


