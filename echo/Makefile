
link: yecho.exe

install:
	@copy yecho.exe $(BINDIR) >NUL
	@if exist yecho.pdb copy yecho.pdb $(SYMDIR) >NUL

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdyecho.pdb
LINKPDB=/Pdb:yecho.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DECHO_VER_MAJOR=$(ECHO_VER_MAJOR) -DECHO_VER_MINOR=$(ECHO_VER_MINOR)

OBJS=\
	 echo.obj         \

compile: $(OBJS)

yecho.exe: $(OBJS) 
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(ECHO_VER_MAJOR).$(ECHO_VER_MINOR) $(LINKPDB) -out:$@

