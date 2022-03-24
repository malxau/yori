

.SUFFIXES: .com .obj .lib

MODULES =  alias.com     \
           chdir.com     \
           color.com     \
           direnv.com    \
           exit.com      \
           false.com     \
           fg.com        \
           history.com   \
           if.com        \
           job.com       \
           pushd.com     \
           rem.com       \
           set.com       \
           setlocal.com  \
           true.com      \
           ver.com       \
           wait.com      \
           ys.com        \
           z.com         \

BUILTINS = alias.obj     \
           chdir.obj     \
           color.obj     \
           direnv.obj    \
           exit.obj      \
           false.obj     \
           fg.obj        \
           history.obj   \
           if.obj        \
           job.obj       \
           pushd.obj     \
           rem.obj       \
           set.obj       \
           setlocal.obj  \
           true.obj      \
           ver.obj       \
           wait.obj      \
           ys.obj        \
           z.obj         \

all: compile link

!INCLUDE "..\config\common.mk"

compile: builtins.lib

builtins.lib: $(BUILTINS) 
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(BUILTINS) -out:$@

COM_LIBS = $(YORICRT) $(YORILIB) $(YORIWIN) $(YORIVER)

!IFDEF _YMAKE_VER
.obj.com: $(COM_LIBS)
!ELSE
.obj.com:
!ENDIF
	@echo $@
	@$(LINK) $(LDFLAGS) $< -dll -version:$(YORI_BASE_VER_MAJOR).$(YORI_BASE_VER_MINOR) $(LIBS) $(COM_LIBS) $(CRTLIB) -def:$(<B).def -out:$@

