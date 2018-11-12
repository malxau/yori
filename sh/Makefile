
BINARIES=yori.exe yorimin.exe oneyori.exe

!INCLUDE "..\config\common.mk"

YORIMINPDB=
YORISTDPDB=

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdyori.pdb
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
	..\for\builtins.lib      \
	..\nice\builtins.lib     \

FULL_BUILTINLIBS=\
	$(STD_BUILTINLIBS)       \
	..\cab\builtins.lib      \
	..\clip\builtins.lib     \
	..\cls\builtins.lib      \
	..\compact\builtins.lib  \
	..\copy\builtins.lib     \
	..\cshot\builtins.lib    \
	..\cut\builtins.lib      \
	..\cvtvt\builtins.lib    \
	..\date\builtins.lib     \
	..\dir\builtins.lib      \
	..\echo\builtins.lib     \
	..\erase\builtins.lib    \
	..\expr\builtins.lib     \
	..\finfo\builtins.lib    \
	..\fscmp\builtins.lib    \
	..\get\builtins.lib      \
	..\grpcmp\builtins.lib   \
	..\help\builtins.lib     \
	..\hexdump\builtins.lib  \
	..\hilite\builtins.lib   \
	..\iconv\builtins.lib    \
	..\initool\builtins.lib  \
	..\intcmp\builtins.lib   \
	..\lines\builtins.lib    \
	..\lsof\builtins.lib     \
	..\mkdir\builtins.lib    \
	..\mklink\builtins.lib   \
	..\more\builtins.lib     \
	..\move\builtins.lib     \
	..\nice\builtins.lib     \
	..\osver\builtins.lib    \
	..\path\builtins.lib     \
	..\pause\builtins.lib    \
	..\pkglib\yoripkg.lib    \
	..\readline\builtins.lib \
	..\repl\builtins.lib     \
	..\rmdir\builtins.lib    \
	..\scut\builtins.lib     \
	..\sdir\builtins.lib     \
	..\setver\builtins.lib   \
	..\sleep\builtins.lib    \
	..\split\builtins.lib    \
	..\start\builtins.lib    \
	..\strcmp\builtins.lib   \
	..\sync\builtins.lib     \
	..\tail\builtins.lib     \
	..\tee\builtins.lib      \
	..\title\builtins.lib    \
	..\touch\builtins.lib    \
	..\type\builtins.lib     \
	..\vol\builtins.lib      \
	..\which\builtins.lib    \
	..\wininfo\builtins.lib  \
	..\winpos\builtins.lib   \
	..\ydbg\builtins.lib     \
	..\ypm\builtins.lib      \

compile: $(OBJS) $(BUILTINTABLE_OBJS)

yorimin.exe: $(OBJS) yorinone.obj
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) yorinone.obj $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) -def:$(@B).def $(MIN_PDB) -out:$@

yori.exe: $(OBJS) yoristd.obj
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) yoristd.obj $(CRTLIB) ..\lib\yorilib.lib $(STD_BUILTINLIBS) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) -def:$(@B).def $(STD_PDB) -out:$@

oneyori.exe: $(OBJS) yorifull.obj
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) yorifull.obj $(CRTLIB) ..\lib\yorilib.lib $(FULL_BUILTINLIBS) -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) -def:$(@B).def $(FULL_PDB) -out:$@

