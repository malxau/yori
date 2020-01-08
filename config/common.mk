
#
# Common makefile include to define build options that apply for all
# subprojects.
#

!INCLUDE "VER.INC"
YORI_BUILD_ID=0

UNICODE=1
DEBUG=0
!IF $(DEBUG)==1
PDB=1
!ELSE
PDB=0
!ENDIF

!IF "$(READCONFIGCACHEFILE)"==""

PROBECOMPILER=1
PROBELINKER=1

LIB32=link.exe -lib -nologo
LINK=link.exe -nologo -incremental:no
MAKE=nmake.exe -nologo

CFLAGS_NOUNICODE=-nologo -W4 -WX -I. -I..\lib -I..\crt -I..\libwin -I..\libdlg -DYORI_VER_MAJOR=$(YORI_VER_MAJOR) -DYORI_VER_MINOR=$(YORI_VER_MINOR) -DYORI_BUILD_ID=$(YORI_BUILD_ID)
LDFLAGS=-OPT:REF
LIBFLAGS=
LIBS=kernel32.lib

#
# Set the correct entrypoint depending on whether we're
# ANSI or Unicode.
#

!IF $(UNICODE)==1
ENTRY=wmainCRTStartup
!ELSE
ENTRY=mainCRTStartup
!ENDIF
YENTRY=ymainCRTStartup

!IF $(DEBUG)==1
CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) -Gy -Z7 -Od -DDBG=1
LDFLAGS=$(LDFLAGS) -DEBUG
!ELSE
CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) -Gy -O1sb1

!IF $(PDB)==1
CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) -Z7
LDFLAGS=$(LDFLAGS) -DEBUG
!ENDIF

!ENDIF # DEBUG

#
# Include and link to the desired CRT.
#

CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) -DMINICRT
LDFLAGS=$(LDFLAGS) -nodefaultlib
CRTLIB=..\crt\yoricrt.lib

!IF $(PROBECOMPILER)==1

#
# Probe for more esoteric aspects of compiler behavior.
# Apparently pragma doesn't cut it for -GS-, and this is
# the most critical one for Minicrt.  MP is a perf
# optimization that only exists on newer compilers, so
# skip the probe on old ones.
#

!IF [$(CC) -GS- 2>&1 | find "unknown" >NUL]>0
CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) -GS-
CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) -GF
!IF [$(CC) -? 2>&1 | find "/MP" >NUL]==0
CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) -MP
!ENDIF
!ELSE
!IF [$(CC) -GF 2>&1 | find "unknown" >NUL]>0
CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) -GF
!ELSE
CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) -Gf
!ENDIF
!ENDIF

#
# Probe for stdcall support.  This should exist on all x86
# compilers, but not x64 compilers.
#

!IF [$(CC) -? 2>&1 | find "Gz __stdcall" >NUL]==0
CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) -Gz
!ENDIF

#
# Probe for -Gs support.  This exists on x86 and x64 but not mips.
#

!IF [$(CC) -Gs9999 2>&1 |find "unknown" >NUL]>0
CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) -Gs9999
!ENDIF

!ENDIF # PROBECOMPILER

#
# Craft the "real" CFLAGS which are typically Unicode.  Non-unicode
# is needed to build parts of the CRT.
#
!IF $(UNICODE)==1
CFLAGS=$(CFLAGS_NOUNICODE) -DUNICODE -D_UNICODE
!ELSE
CFLAGS=$(CFLAGS_NOUNICODE)
!ENDIF

!IF $(PROBELINKER)==1

#
# Visual C 2003 really wants this and isn't satisfied
# with a pragma, so yet another probe
#

!IF [$(LINK) -OPT:NOWIN98 2>&1 | find "NOWIN98" >NUL]>0
LDFLAGS=$(LDFLAGS) -OPT:NOWIN98
!ENDIF

!IF [$(LINK) -OPT:ICF 2>&1 | find "ICF" >NUL]>0
LDFLAGS=$(LDFLAGS) -OPT:ICF
!ENDIF

#
# Try to detect which architecture the compiler is generating.  This is done
# to help the subsystem version detection, below.  If we don't guess it's
# not fatal, the linker will figure it out in the end.
#
!IF [$(CC) 2>&1 | find "for 80x86" >NUL]==0
LDFLAGS=$(LDFLAGS) -MACHINE:IX86
!ELSE
!IF [$(CC) 2>&1 | find "for x86" >NUL]==0
LDFLAGS=$(LDFLAGS) -MACHINE:X86
!ELSE
!IF [$(CC) 2>&1 | find "for x64" >NUL]==0
LDFLAGS=$(LDFLAGS) -MACHINE:X64
!ELSE
!IF [$(CC) 2>&1 | find "for AMD64" >NUL]==0
LDFLAGS=$(LDFLAGS) -MACHINE:AMD64
!ELSE
!IF [$(CC) 2>&1 | find "for ARM64" >NUL]==0
LDFLAGS=$(LDFLAGS) -MACHINE:ARM64
!ELSE
!IF [$(CC) 2>&1 | find "for Itanium" >NUL]==0
LDFLAGS=$(LDFLAGS) -MACHINE:IA64
!ELSE
!IF [$(CC) 2>&1 | find "for ARM" >NUL]==0
LDFLAGS=$(LDFLAGS) -MACHINE:ARM
# Add back msvcrt to provide 64 bit math assembly
CRTLIB=$(CRTLIB) msvcrt.lib libvcruntime.lib
!ELSE
!IF [$(CC) 2>&1 | find "for MIPS" >NUL]==0
LDFLAGS=$(LDFLAGS) -MACHINE:MIPS
# Add back msvcrt to provide 64 bit math assembly
CRTLIB=$(CRTLIB) msvcrt.lib
!ENDIF # MIPS
!ENDIF # ARM (32)
!ENDIF # Itanium
!ENDIF # ARM64
!ENDIF # AMD64
!ENDIF # x64
!ENDIF # x86
!ENDIF # 80x86

#
# Look for the oldest subsystem version the linker is willing to generate
# for us.  Because this app with minicrt will run back to NT 3.1, the primary
# limit is the linker.  Without minicrt we're at the mercy of whatever CRT is
# there, so just trust the defaults.  For execution efficiency, these checks
# are structured as a tree:
#
#                             5.0?
#                           /      \
#                         /          \
#                       /              \
#                   4.0?                5.2?
#                 /      \            /      \
#             3.10?        5.0    5.1?         6.0?
#           /      \             /    \       /    \
#        3.10      4.0         5.1    5.2   6.0   default
#

!IF [$(LINK) $(LDFLAGS) -SUBSYSTEM:CONSOLE,5.0 2>&1 | find "default subsystem" >NUL]>0
!IF [$(LINK) $(LDFLAGS) -SUBSYSTEM:CONSOLE,4.0 2>&1 | find "default subsystem" >NUL]>0
!IF [$(LINK) $(LDFLAGS) -SUBSYSTEM:CONSOLE,3.10 2>&1 | find "default subsystem" >NUL]>0
LDFLAGS=$(LDFLAGS) -SUBSYSTEM:CONSOLE,3.10
!ELSE  # !3.10
LDFLAGS=$(LDFLAGS) -SUBSYSTEM:CONSOLE,4.0
!ENDIF # 3.10
!ELSE  # !4.0
LDFLAGS=$(LDFLAGS) -SUBSYSTEM:CONSOLE,5.0
!ENDIF # 4.0
!ELSE  # !5.0
!IF [$(LINK) $(LDFLAGS) -SUBSYSTEM:CONSOLE,5.2 2>&1 | find "default subsystem" >NUL]>0
!IF [$(LINK) $(LDFLAGS) -SUBSYSTEM:CONSOLE,5.1 2>&1 | find "default subsystem" >NUL]>0
LDFLAGS=$(LDFLAGS) -SUBSYSTEM:CONSOLE,5.1
!ELSE  # !5.1
LDFLAGS=$(LDFLAGS) -SUBSYSTEM:CONSOLE,5.2
!ENDIF # 5.1
!ELSE  # !5.2
!IF [$(LINK) $(LDFLAGS) -SUBSYSTEM:CONSOLE,6.0 2>&1 | find "default subsystem" >NUL]>0
LDFLAGS=$(LDFLAGS) -SUBSYSTEM:CONSOLE,6.0
!ELSE
LDFLAGS=$(LDFLAGS) -SUBSYSTEM:CONSOLE
!ENDIF # 6.0
!ENDIF # 5.2
!ENDIF # 5.0

!ENDIF # PROBELINKER

FOR=for
FOR_ST=for
MKDIR=mkdir
RMDIR=rmdir

!IF [yfor.exe -? >NUL 2>&1]==0
FOR=yfor -c -p %NUMBER_OF_PROCESSORS%
FOR_ST=yfor -c
!ELSE
!IF [oneyori.exe -c for -? >NUL 2>&1]==0
FOR=oneyori -c for -c -p %NUMBER_OF_PROCESSORS%
FOR_ST=oneyori -c for -c
!ENDIF
!ENDIF

!IF [ymkdir.exe -? >NUL 2>&1]==0
MKDIR=ymkdir
!ENDIF
!IF [yrmdir.exe -? >NUL 2>&1]==0
RMDIR=yrmdir
!ENDIF


!IF "$(WRITECONFIGCACHEFILE)"!=""
writeconfigcache:
	@echo Generating config cache...
	@echo CC=$(CC) >$(WRITECONFIGCACHEFILE)
	@echo CFLAGS=$(CFLAGS) >>$(WRITECONFIGCACHEFILE)
	@echo CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) >>$(WRITECONFIGCACHEFILE)
	@echo CRTLIB=$(CRTLIB) >>$(WRITECONFIGCACHEFILE)
	@echo ENTRY=$(ENTRY) >>$(WRITECONFIGCACHEFILE)
	@echo YENTRY=$(YENTRY) >>$(WRITECONFIGCACHEFILE)
	@echo FOR=$(FOR) >>$(WRITECONFIGCACHEFILE)
	@echo FOR_ST=$(FOR_ST) >>$(WRITECONFIGCACHEFILE)
	@echo LDFLAGS=$(LDFLAGS) >>$(WRITECONFIGCACHEFILE)
	@echo LIB32=$(LIB32) >>$(WRITECONFIGCACHEFILE)
	@echo LIBFLAGS=$(LIBFLAGS) >>$(WRITECONFIGCACHEFILE)
	@echo LIBS=$(LIBS) >>$(WRITECONFIGCACHEFILE)
	@echo LINK=$(LINK) >>$(WRITECONFIGCACHEFILE)
	@echo MAKE=$(MAKE) >>$(WRITECONFIGCACHEFILE)
	@echo MKDIR=$(MKDIR) >>$(WRITECONFIGCACHEFILE)
	@echo RMDIR=$(RMDIR) >>$(WRITECONFIGCACHEFILE)

!ENDIF

!ELSE # READCONFIGCACHEFILE
!INCLUDE "$(READCONFIGCACHEFILE)"
!ENDIF
!IF "$(WRITECONFIGCACHEFILE)"==""

link: $(BINARIES) $(MODULES) compile

!IFDEF _NMAKE_VER
.c.obj::
!ELSE
.c.obj:
!ENDIF
	@$(CC) $(CFLAGS) -c $<

.rc.obj:
	@echo $(**F)
	@if exist $@ erase $@
	@$(RC) /fo$(@B).res $(RCFLAGS) $** >NUL
	@if not exist $@ ren $(@B).res $@

clean:
	-@erase if.com >NUL 2>NUL
	@if exist *.exe erase *.exe
	@if exist *.com erase *.com
	@if exist *.dll erase *.dll
	@if exist *.obj erase *.obj
	@if exist *.pdb erase *.pdb
	@if exist *.lib erase *.lib
	@if exist *.exp erase *.exp
	@if exist *.res erase *.res
	@if exist *~ erase *~
	@if exist *.exe.manifest erase *.exe.manifest

install:
	@if not "$(BINARIES)."=="." for %%i in ($(BINARIES)) do @copy %%i $(BINDIR) >NUL
	@if not "$(BINARIES)."=="." for %%i in ($(BINARIES)) do @if exist %%~dpni.pdb copy %%~dpni.pdb $(SYMDIR) >NUL
	@if not "$(MODULES)."=="." for %%i in ($(MODULES)) do @copy %%i $(MODDIR) >NUL
	@if not "$(MODULES)."=="." for %%i in ($(MODULES)) do @if exist %%~dpni.pdb copy %%~dpni.pdb $(SYMDIR) >NUL
	@if not "$(INITFILES)."=="." for %%i in ($(INITFILES)) do @copy %%i $(BINDIR)\YoriInit.d >NUL
!ENDIF
