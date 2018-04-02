
compile: yoricrt.lib

link:

install:

!INCLUDE "..\config\common.mk"

OBJS=\
     mem.obj      \
     rand.obj     \
     string.obj   \
     stringw.obj  \
     ep_cons.obj  \
     ep_consw.obj \
     ep_dll.obj   \

BIGMATH_OBJS=bigdiv.obj  \
			 bigdvrm.obj \
			 bigmul.obj  \
			 bigrem.obj  \
			 bigshl.obj  \
			 bigshr.obj  \

ML=ml.exe

# 
# Check is ml.exe is present.  If so, the bigmath objects can be
# rebuilt, otherwise use prebuilt ones.
#

!IF [$(ML) 2>&1 | find "Microsoft" >NUL]>0
ML=
!ENDIF

#
# Check if we're compiling for x86 and if so include bigmath. This isn't
# needed on 64 bit systems and for non-x86 32 bit systems we don't have
# an implementation anyway.  This check is correct for Visual C++ 2 and
# newer; older systems don't have 64 bit math anyway.
#

!IF [$(CC) 2>&1 | find "x86" >NUL]==0
OBJS=$(OBJS) $(BIGMATH_OBJS)
!ENDIF

yoricrt.lib: $(OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(OBJS) -out:$@

!IFDEF _NMAKE_VER
.c.obj::
!ELSE
.c.obj:
!ENDIF
	@$(CC) $(CFLAGS_NOUNICODE) -c $<

!IF "$(ML)"!=""
.asm.obj:
	@$(ML) /nologo /coff /c $<
!ELSE
.asm.obj:
	@echo i386\$@ (copy binary)
	@copy i386\$@ .
!ENDIF

