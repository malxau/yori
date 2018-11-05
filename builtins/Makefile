

.SUFFIXES: .com .obj .lib

MODULES =  alias.com     \
           builtin.com   \
           chdir.com     \
           color.com     \
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
           builtin.obj   \
           chdir.obj     \
           color.obj     \
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

compile: builtins.lib

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
CFLAGS=$(CFLAGS) /Fdbuiltins.pdb
!ENDIF

builtins.lib: $(BUILTINS) 
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(BUILTINS) -out:$@

.obj.com:
	@echo $@
	@$(LINK) $(LDFLAGS) $< -dll -version:$(YORI_VER_MAJOR).$(YORI_VER_MINOR) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -def:$(<B).def -out:$@

