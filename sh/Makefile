
BINARIES=yori.exe yorimin.exe oneyori.exe
INITFILES=YoriInit.d\*

!INCLUDE "..\config\common.mk"

YORIMINPDB=
YORISTDPDB=

!IF $(PDB)==1
MIN_PDB=/Pdb:yorimin.pdb
STD_PDB=/Pdb:yori.pdb
FULL_PDB=/Pdb:oneyori.pdb
!ENDIF

OBJS=\
	alias.obj        \
	api.obj          \
	builtin.obj      \
	cmdbuf.obj       \
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
	window.obj       \
	yori.obj         \

BUILTINTABLE_OBJS=\
	yoristd.obj      \
	yorinone.obj     \
	yorifull.obj     \

STD_BUILTINLIBS=\
	..\builtins\builtins.lib \
	..\libwin\yoriwin.lib    \
	..\echo\builtins.lib     \
	..\for\builtins.lib      \
	..\intcmp\builtins.lib   \
	..\nice\builtins.lib     \
	..\strcmp\builtins.lib   \

FULL_BUILTINLIBS=\
	$(STD_BUILTINLIBS)       \
	yorifull.lib             \

compile: $(OBJS) $(BUILTINTABLE_OBJS)

yorimin.exe: $(OBJS) yorinone.obj yorimin.def ..\lib\yorilib.lib ..\crt\yoricrt.lib
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) yorinone.obj $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) -def:$(@B).def $(MIN_PDB) -out:$@

yori.exe: $(OBJS) yoristd.obj yori.def ..\lib\yorilib.lib ..\crt\yoricrt.lib $(STD_BUILTINLIBS)
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) yoristd.obj $(CRTLIB) ..\lib\yorilib.lib $(STD_BUILTINLIBS) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) -def:$(@B).def $(STD_PDB) -out:$@

!IFDEF _YMAKE_VER
ONEYORI_DEPLIST=@yorifull.lst
!ENDIF

oneyori.exe: $(OBJS) yorifull.obj oneyori.def yorifull.lst ..\lib\yorilib.lib ..\crt\yoricrt.lib $(STD_BUILTINLIBS) $(ONEYORI_DEPLIST)
	@$(LIB32) $(LIBFLAGS) @yorifull.lst /out:yorifull.lib
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) yorifull.obj $(CRTLIB) ..\lib\yorilib.lib $(FULL_BUILTINLIBS) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) -def:$(@B).def $(FULL_PDB) -out:$@

