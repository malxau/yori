

link: yori.exe yorimin.exe

install:
	@if not exist $(BINDIR) mkdir $(BINDIR)
	@if not exist $(SYMDIR) mkdir $(SYMDIR)
	@copy yori.exe $(BINDIR) >NUL
	@if exist yori.pdb copy yori.pdb $(SYMDIR) >NUL
	@copy yorimin.exe $(BINDIR) >NUL
	@if exist yorimin.pdb copy yorimin.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

YORIMINPDB=
YORIFULLPDB=

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdyori.pdb
YORIMINPDB=/Pdb:yorimin.pdb
YORIFULLPDB=/Pdb:yori.pdb
!ENDIF

OBJS=\
	alias.obj        \
	api.obj          \
	builtin.obj      \
	clip.obj         \
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
	yorifull.obj     \
	yorinone.obj     \

BUILTINLIBS=\
	..\builtins\builtins.lib \
	..\for\builtins.lib      \

compile: $(OBJS) $(BUILTINTABLE_OBJS)

yorimin.exe: $(OBJS) yorinone.obj
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) yorinone.obj $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) -def:$(@B).def $(YORIPDB) -out:$@

yori.exe: $(OBJS) yorifull.obj
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) yorifull.obj $(CRTLIB) ..\lib\yorilib.lib $(BUILTINLIBS) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) -def:$(@B).def $(YORIFULLPDB) -out:$@

