
BINARIES=yori.exe yorimin.exe oneyori.exe
INITFILES=YoriInit.d\*

!INCLUDE "..\config\common.mk"
YORI_BIN_VER_MAJOR=$(YORI_VER_MAJOR)
YORI_BIN_VER_MINOR=$(YORI_VER_MINOR)

MIN_PDB=/Pdb:yorimin.pdb
STD_PDB=/Pdb:yori.pdb
FULL_PDB=/Pdb:oneyori.pdb

OBJS=\
	alias.obj        \
	api.obj          \
	builtin.obj      \
	complete.obj     \
	env.obj          \
	exec.obj         \
	history.obj      \
	input.obj        \
	job.obj          \
	main.obj         \
	parse.obj        \
	prompt.obj       \
	restart.obj      \
	wait.obj         \
	window.obj       \

BUILTINTABLE_OBJS=\
	yoristd.obj      \
	yorinone.obj     \
	yorifull.obj     \

STD_BUILTINLIBS=\
	..\builtins\builtins.lib \
	$(YORIWIN)    \
	..\echo\builtins.lib     \
	..\for\builtins.lib      \
	..\intcmp\builtins.lib   \
	..\nice\builtins.lib     \
	..\strcmp\builtins.lib   \

FULL_BUILTINLIBS=\
	$(STD_BUILTINLIBS)       \
	yorifull.lib             \

STD_RSRC=\
	yori.obj         \

FULL_RSRC=\
	oneyori.obj      \

compile: $(OBJS) $(BUILTINTABLE_OBJS)

yorimin.exe: $(OBJS) $(STD_RSRC) yorinone.obj yorimin.def $(YORILIBS) $(YORISH)
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(STD_RSRC) yorinone.obj $(YORILIBS) $(EXTERNLIBS) $(YORISH) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) -def:$(@B).def $(MIN_PDB) -out:$@

yori.exe: $(OBJS) $(STD_RSRC) yoristd.obj yori.def $(YORILIBS) $(YORISH) $(STD_BUILTINLIBS)
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(STD_RSRC) yoristd.obj $(YORILIBS) $(EXTERNLIBS) $(YORISH) $(STD_BUILTINLIBS) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) -def:$(@B).def $(STD_PDB) -out:$@

!IFDEF _YMAKE_VER
ONEYORI_DEPLIST=@yorifull.lst
!ENDIF

oneyori.exe: $(OBJS) $(FULL_RSRC) yorifull.obj oneyori.def yorifull.lst $(YORILIBS) $(YORISH) $(STD_BUILTINLIBS) $(ONEYORI_DEPLIST)
	@$(LIB32) $(LIBFLAGS) @yorifull.lst /out:yorifull.lib
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(FULL_RSRC) yorifull.obj $(YORILIBS) $(EXTERNLIBS) $(YORISH) $(FULL_BUILTINLIBS) $(FDILIB) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) -def:$(@B).def $(FULL_PDB) -out:$@

