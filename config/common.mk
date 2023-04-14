
#
# Common makefile include to define build options that apply for all
# subprojects.
#

!IFNDEF YORI_BASE_VER_MAJOR
!INCLUDE "VER.MK"
!ENDIF
YORI_BUILD_ID=0

DEBUG=0
ANALYZE=0
KERNELBASE=0

!IFNDEF FDI
FDI=0
!ENDIF

!IFNDEF BINDIR
BINDIR=
!ENDIF
!IFNDEF MODDIR
MODDIR=
!ENDIF
!IFNDEF SYMDIR
SYMDIR=
!ENDIF
!IFNDEF MODULES
MODULES=
!ENDIF
!IFNDEF INITFILES
INITFILES=
!ENDIF
!IFNDEF BINARIES
BINARIES=
!ENDIF
!IFNDEF READCONFIGCACHEFILE
READCONFIGCACHEFILE=
!ENDIF
!IFNDEF WRITECONFIGCACHEFILE
WRITECONFIGCACHEFILE=
!ENDIF

!IF "$(READCONFIGCACHEFILE)"==""
!IFNDEF YORI_VARIABLES_DEFINED

PROBECOMPILER=1
PROBELINKER=1

LIB32=link.exe -lib -nologo
LINK=link.exe -nologo -incremental:no
MAKE=nmake.exe -nologo

CFLAGS_NOUNICODE=-nologo -W4 -WX -I. -I..\lib -I..\crt -I..\libwin -I..\libdlg -I..\libsh -DYORI_VER_MAJOR=$(YORI_VER_MAJOR) -DYORI_VER_MINOR=$(YORI_VER_MINOR) -DYORI_BUILD_ID=$(YORI_BUILD_ID)
LDFLAGS=-OPT:REF
LIBFLAGS=

!IF $(KERNELBASE)==1
YORILIBS=..\kernelbase\kernelbase.lib
EXTERNLIBS=
!ELSE
YORILIBS=
EXTERNLIBS=kernel32.lib
!ENDIF

ENTRY=wmainCRTStartup
YENTRY=ymainCRTStartup

!IF $(ANALYZE)==1
CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) -Wall -analyze:plugin EspXEngine.dll
!ENDIF

!IF $(DEBUG)==1
CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) -Gy -Z7 -Od -DDBG=1
LDFLAGS=$(LDFLAGS) -DEBUG
!ELSE
CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) -Gy -O1sb1

!ENDIF # DEBUG

#
# Include and link to the desired CRT.
#

CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) -DMINICRT
LDFLAGS=$(LDFLAGS) -nodefaultlib
YORILIBS=$(YORILIBS) ..\crt\yoricrt.lib ..\lib\yorilib.lib
YORIDLG=..\libdlg\yoridlg.lib
YORIPKG=..\pkglib\yoripkg.lib
YORISH=..\libsh\yorish.lib
YORIVER=..\lib\yoriver.obj
YORIWIN=..\libwin\yoriwin.lib

FDILIB=
!IF $(FDI)==1
CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) -DYORI_FDI_SUPPORT
FDILIB=fdi.lib
!ENDIF

!IF $(PROBECOMPILER)==1

#
# Probe for more esoteric aspects of compiler behavior.
# Apparently pragma doesn't cut it for -GS-, and this is
# the most critical one for Minicrt.  MP is a perf
# optimization that only exists on newer compilers, so
# skip the probe on old ones.
#

!IF [$(CC) -GS- 2>&1 | find "D4002" >NUL]>0
CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) -GS-
CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) -GF
!IFNDEF _YMAKE_VER
!IF [$(CC) -? 2>&1 | find "/MP" >NUL]==0
CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) -MP
!ENDIF
!ENDIF
!ELSE
!IF [$(CC) -GF 2>&1 | find "D4002" >NUL]>0
CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) -GF
!ELSE
CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) -Gf
!ENDIF
!ENDIF

#
# Probe for stdcall support.  This should exist on all x86
# compilers, but not x64 compilers.
#

!IF [$(CC) -? 2>&1 | find "/Gz" >NUL]==0
CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) -Gz
!ENDIF

#
# Probe for -Gs support.  This exists on x86 and x64 but not mips.
#

!IF [$(CC) -Gs9999 2>&1 |find "D4002" >NUL]>0
CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) -Gs9999
!ENDIF

!ENDIF # PROBECOMPILER

#
# Craft the "real" CFLAGS which are Unicode.  Non-unicode is needed to build
# parts of the CRT.
#
CFLAGS=$(CFLAGS_NOUNICODE) -DUNICODE -D_UNICODE

!IF $(PROBELINKER)==1

#
# Visual C 2003 really wants NOWIN98 and isn't satisfied
# with a pragma, so yet another probe
#

!IF [$(LINK) -OPT:ICF 2>&1 | find "ICF" >NUL]>0
LDFLAGS=$(LDFLAGS) -OPT:ICF
!IF [$(LINK) -OPT:NOWIN98 2>&1 | find "NOWIN98" >NUL]>0
LDFLAGS=$(LDFLAGS) -OPT:NOWIN98
!ENDIF
!ENDIF

#
# Try to detect which architecture the compiler is generating.  This is done
# to help the subsystem version detection, below.  If we don't guess it's
# not fatal, the linker will figure it out in the end.
#
MINOS=310
!IF [$(CC) --version 2>&1 | find "002 :" >NUL]==0 # MSVC or Clang
!IF [$(CC) 2>&1 | find "x86" >NUL]==0
!IF [$(CC) 2>&1 | find "80x86" >NUL]==0
MACHINE=IX86
!ELSE
MACHINE=X86
!ENDIF # 80x86
ARCH=win32
!ELSE
!IF [$(CC) 2>&1 | find "x64" >NUL]==0
MACHINE=X64
MINOS=520
ARCH=amd64
!ELSE
!IF [$(CC) 2>&1 | find "AMD64" >NUL]==0
MACHINE=AMD64
MINOS=520
ARCH=amd64
!ELSE
!IF [$(CC) 2>&1 | find "ARM64" >NUL]==0
MACHINE=ARM64
MINOS=1000
ARCH=arm64
!ELSE
!IF [$(CC) 2>&1 | find "Itanium" >NUL]==0
MACHINE=IA64
MINOS=520
ARCH=ia64
!ELSE
!IF [$(CC) 2>&1 | find "IA-64" >NUL]==0
MACHINE=IA64
MINOS=520
ARCH=ia64
!ELSE
!IF [$(CC) 2>&1 | find "ARM" >NUL]==0
MACHINE=ARM
# Add back msvcrt to provide 64 bit math assembly
EXTERNLIBS=$(EXTERNLIBS) msvcrt.lib libvcruntime.lib
MINOS=800
ARCH=arm
!ELSE
!IF [$(CC) 2>&1 | find "MIPS" >NUL]==0
MACHINE=MIPS
# Add back msvcrt to provide 64 bit math assembly
EXTERNLIBS=$(EXTERNLIBS) msvcrt.lib
ARCH=mips
!ELSE
!IF [$(CC) 2>&1 | find "PowerPC" >NUL]==0
MACHINE=PPC
# Add back msvcrt to provide 64 bit math assembly
EXTERNLIBS=$(EXTERNLIBS) msvcrt.lib
ARCH=ppc
!ELSE
!IF [$(CC) 2>&1 | find "Alpha" >NUL]==0
MACHINE=ALPHA
# Add back msvcrt to provide 64 bit math assembly
EXTERNLIBS=$(EXTERNLIBS) msvcrt.lib
ARCH=axp
!ENDIF # AXP
!ENDIF # PPC
!ENDIF # MIPS
!ENDIF # ARM (32)
!ENDIF # IA-64
!ENDIF # Itanium
!ENDIF # ARM64
!ENDIF # AMD64
!ENDIF # x64
!ENDIF # x86
!ELSE  # MSVC/Clang, clang is below
!IF [$(CC) --version 2>&1 | find "x86-64-windows" >NUL]==0
MACHINE=X64
MINOS=520
ARCH=amd64
!ELSE
!IF [$(CC) --version 2>&1 | find "x86-pc-windows" >NUL]==0
MACHINE=X86
ARCH=win32
!ELSE
!IF [$(CC) --version 2>&1 | find "aarch64-pc-windows" >NUL]==0
MACHINE=ARM64
MINOS=1000
ARCH=arm64
!ENDIF # aarch64
!ENDIF # x86
!ENDIF # x86-64
!ENDIF # MSVC/Clang

LDFLAGS=$(LDFLAGS) -MACHINE:$(MACHINE)

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

# 32 bit builds need to probe 5.0 and lower.  64 bit builds can jump
# straight to 5.2 which is the oldest 64 bit OS.
!IF $(MINOS)<520
!IF [$(LINK) $(LDFLAGS) -SUBSYSTEM:CONSOLE,5.0 2>&1 | find "LNK4010" >NUL]>0
!IF [$(LINK) $(LDFLAGS) -SUBSYSTEM:CONSOLE,4.0 2>&1 | find "LNK4010" >NUL]>0
!IF [$(LINK) $(LDFLAGS) -SUBSYSTEM:CONSOLE,3.10 2>&1 | find "LNK4010" >NUL]>0
LDFLAGS=$(LDFLAGS) -SUBSYSTEM:CONSOLE,3.10
!ELSE  # !3.10
LDFLAGS=$(LDFLAGS) -SUBSYSTEM:CONSOLE,4.0
!ENDIF # 3.10
!ELSE  # !4.0
LDFLAGS=$(LDFLAGS) -SUBSYSTEM:CONSOLE,5.0
!ENDIF # 4.0
!ELSE  # !5.0
!IF [$(LINK) $(LDFLAGS) -SUBSYSTEM:CONSOLE,5.2 2>&1 | find "LNK4010" >NUL]>0
!IF [$(LINK) $(LDFLAGS) -SUBSYSTEM:CONSOLE,5.1 2>&1 | find "LNK4010" >NUL]>0
LDFLAGS=$(LDFLAGS) -SUBSYSTEM:CONSOLE,5.1
!ELSE  # !5.1
LDFLAGS=$(LDFLAGS) -SUBSYSTEM:CONSOLE,5.2
!ENDIF # 5.1
!ELSE  # !5.2
!IF [$(LINK) $(LDFLAGS) -SUBSYSTEM:CONSOLE,6.0 2>&1 | find "LNK4010" >NUL]>0
LDFLAGS=$(LDFLAGS) -SUBSYSTEM:CONSOLE,6.0
!ELSE
LDFLAGS=$(LDFLAGS) -SUBSYSTEM:CONSOLE
!ENDIF # 6.0
!ENDIF # 5.2
!ENDIF # 5.0
!ELSE  # MINOS<520 aka 64 bit build
!IF [$(LINK) $(LDFLAGS) -SUBSYSTEM:CONSOLE,5.2 2>&1 | find "LNK4010" >NUL]>0
LDFLAGS=$(LDFLAGS) -SUBSYSTEM:CONSOLE,5.2
!ELSE  # !5.2
!IF [$(LINK) $(LDFLAGS) -SUBSYSTEM:CONSOLE,6.0 2>&1 | find "LNK4010" >NUL]>0
LDFLAGS=$(LDFLAGS) -SUBSYSTEM:CONSOLE,6.0
!ELSE
LDFLAGS=$(LDFLAGS) -SUBSYSTEM:CONSOLE
!ENDIF # 6.0
!ENDIF # 5.2
!ENDIF # MINOS<520

!ENDIF # PROBELINKER

FOR=for
FOR_ST=for
MKDIR=mkdir
RMDIR=rmdir

!IFNDEF _YMAKE_VER
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
!ENDIF


!IF "$(WRITECONFIGCACHEFILE)"!=""
writeconfigcache:
	@echo Generating config cache...
	@echo ARCH=$(ARCH) >$(WRITECONFIGCACHEFILE)
	@echo CC=$(CC) >$(WRITECONFIGCACHEFILE)
	@echo CFLAGS=$(CFLAGS) >>$(WRITECONFIGCACHEFILE)
	@echo CFLAGS_NOUNICODE=$(CFLAGS_NOUNICODE) >>$(WRITECONFIGCACHEFILE)
	@echo ENTRY=$(ENTRY) >>$(WRITECONFIGCACHEFILE)
	@echo EXTERNLIBS=$(EXTERNLIBS) >>$(WRITECONFIGCACHEFILE)
	@echo YENTRY=$(YENTRY) >>$(WRITECONFIGCACHEFILE)
	@echo FOR=$(FOR) >>$(WRITECONFIGCACHEFILE)
	@echo FOR_ST=$(FOR_ST) >>$(WRITECONFIGCACHEFILE)
	@echo LDFLAGS=$(LDFLAGS) >>$(WRITECONFIGCACHEFILE)
	@echo LIB32=$(LIB32) >>$(WRITECONFIGCACHEFILE)
	@echo LIBFLAGS=$(LIBFLAGS) >>$(WRITECONFIGCACHEFILE)
	@echo LINK=$(LINK) >>$(WRITECONFIGCACHEFILE)
	@echo MACHINE=$(MACHINE) >>$(WRITECONFIGCACHEFILE)
	@echo MAKE=$(MAKE) >>$(WRITECONFIGCACHEFILE)
	@echo MKDIR=$(MKDIR) >>$(WRITECONFIGCACHEFILE)
	@echo RMDIR=$(RMDIR) >>$(WRITECONFIGCACHEFILE)
	@echo YORIDLG=$(YORIDLG) >>$(WRITECONFIGCACHEFILE)
	@echo YORILIBS=$(YORILIBS) >>$(WRITECONFIGCACHEFILE)
	@echo YORIPKG=$(YORIPKG) >>$(WRITECONFIGCACHEFILE)
	@echo YORISH=$(YORISH) >>$(WRITECONFIGCACHEFILE)
	@echo YORIVER=$(YORIVER) >>$(WRITECONFIGCACHEFILE)
	@echo YORIWIN=$(YORIWIN) >>$(WRITECONFIGCACHEFILE)

!ENDIF

# NMAKE-based builds perform links top level before installs.
# YMAKE-based builds need dependency information to link before install.

!IFDEF _YMAKE_VER
INSTALL_DEPENDENCIES=link $(BINDIR) $(SYMDIR) $(MODDIR) $(BINDIR)\YoriInit.d
!ENDIF

YORI_VARIABLES_DEFINED=1
!ENDIF # YORI_VARIABLES_DEFINED
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
	@$(RC) /fo$(@B).res $(RCFLAGS) /d YORI_VER_MAJOR=$(YORI_BASE_VER_MAJOR) /d YORI_VER_MINOR=$(YORI_BASE_VER_MINOR) /d YORI_BIN_VER_MAJOR=$(YORI_BIN_VER_MAJOR) /d YORI_BIN_VER_MINOR=$(YORI_BIN_VER_MINOR) $** >NUL
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
	@if exist *.pru erase *.pru
	@if exist *~ erase *~
	@if exist *.exe.manifest erase *.exe.manifest

install: $(INSTALL_DEPENDENCIES)
	@if not "$(BINARIES)."=="." for %%i in ($(BINARIES)) do @copy %%i $(BINDIR) >NUL
	@if not "$(BINARIES)."=="." for %%i in ($(BINARIES)) do @if exist %%~dpni.pdb copy %%~dpni.pdb $(SYMDIR) >NUL
	@if not "$(MODULES)."=="." for %%i in ($(MODULES)) do @copy %%i $(MODDIR) >NUL
	@if not "$(MODULES)."=="." for %%i in ($(MODULES)) do @if exist %%~dpni.pdb copy %%~dpni.pdb $(SYMDIR) >NUL
	@if not "$(INITFILES)."=="." for %%i in ($(INITFILES)) do @copy %%i $(BINDIR)\YoriInit.d >NUL

!ENDIF
