
BINARIES=yshutdn.exe

!INCLUDE "..\config\common.mk"

LINKPDB=/Pdb:yshutdn.pdb

BIN_OBJS=\
	 shutdn.obj         \

MOD_OBJS=\
	 mshut.obj     \

compile: $(BIN_OBJS) builtins.lib

yshutdn.exe: $(BIN_OBJS) $(YORILIBS) $(YORIVER)
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(YORILIBS) $(EXTERNLIBS) $(YORIVER) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LINKPDB) -out:$@

mshut.obj: shutdn.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ shutdn.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
