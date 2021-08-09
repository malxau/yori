
ANALYZE=0
ARCH=win32
DEBUG=0
FDI=0
PDB=1
YORI_BUILD_ID=0

!IF [$(CC) -? 2>&1 | findstr /C:"x64" >NUL 2>&1]==0
ARCH=amd64
!ELSE
!IF [$(CC) -? 2>&1 | findstr /C:"AMD64" >NUL 2>&1]==0
ARCH=amd64
!ELSE
!IF [$(CC) -? 2>&1 | findstr /C:"ARM64" >NUL 2>&1]==0
ARCH=arm64
!ELSE
!IF [$(CC) -? 2>&1 | findstr /C:"ARM" >NUL 2>&1]==0
ARCH=arm
!ELSE
!IF [$(CC) -? 2>&1 | findstr /C:"Itanium" >NUL 2>&1]==0
ARCH=ia64
!ELSE
!IF [$(CC) -? 2>&1 | findstr /C:"MIPS" >NUL 2>&1]==0
ARCH=mips
!ENDIF # MIPS
!ENDIF # Itanium
!ENDIF # ARM (32)
!ENDIF # ARM64
!ENDIF # AMD64
!ENDIF # x64

BINDIR_ROOT=bin\$(ARCH)
SYMDIR_ROOT=sym\$(ARCH)
MODDIR_ROOT=bin\$(ARCH)\modules

BINDIR=..\$(BINDIR_ROOT)
SYMDIR=..\$(SYMDIR_ROOT)
MODDIR=..\$(MODDIR_ROOT)

BUILD=$(MAKE) -nologo ANALYZE=$(ANALYZE) DEBUG=$(DEBUG) FDI=$(FDI) PDB=$(PDB) YORI_BUILD_ID=$(YORI_BUILD_ID) BINDIR=$(BINDIR) SYMDIR=$(SYMDIR) MODDIR=$(MODDIR)

CURRENTTIME=REM
!IFNDEF _YMAKE_VER
WRITECONFIGCACHEFILE=cache.mk
!ENDIF

all: all.real

!INCLUDE "config\common.mk"

!IF "$(FOR)"=="for"
STARTCMD=
!ELSE
STARTCMD="
!ENDIF

!IF [ydate.exe -? >NUL 2>&1]==0
CURRENTTIME=echo. & echo For: $(FOR) & ydate $$HOUR$$:$$MIN$$:$$SEC$$ & echo.
!ENDIF

SHDIRS=sh      \

DIRS=crt       \
     lib       \
     libwin    \
     libdlg    \
     libsh     \
     builtins  \
     assoc     \
     attrib    \
     cab       \
     cal       \
     charmap   \
     clip      \
     clmp      \
     cls       \
     co        \
     compact   \
     copy      \
     cpuinfo   \
     cshot     \
     cut       \
     cvtvt     \
     date      \
     df        \
     dir       \
     dircase   \
     du        \
     echo      \
     edit      \
     env       \
     envdiff   \
     erase     \
     err       \
     expr      \
     finfo     \
     for       \
     fscmp     \
     get       \
     grpcmp    \
     hash      \
     help      \
     hexdump   \
     hilite    \
     iconv     \
     initool   \
     intcmp    \
     kill      \
     lines     \
     lsof      \
     make      \
     mem       \
     mkdir     \
     mklink    \
     more      \
     mount     \
     move      \
     nice      \
     osver     \
     path      \
     pause     \
     pkg       \
     pkglib    \
     procinfo  \
     ps        \
     readline  \
     repl      \
     rmdir     \
     scut      \
     sdir      \
     setver    \
     shutdn    \
     sleep     \
     slmenu    \
     split     \
     sponge    \
     start     \
     strcmp    \
     stride    \
     sync      \
     tail      \
     tee       \
     timethis  \
     title     \
     touch     \
     type      \
     vhdtool   \
     vol       \
     which     \
     wininfo   \
     winpos    \
     ydbg      \
     ypm       \
     ysetup    \
     yui       \

!IFDEF _YMAKE_VER
$(BINDIR_ROOT):
	@-$(MKDIR) $(BINDIR_ROOT) 

$(BINDIR_ROOT)\YoriInit.d:
	@-$(MKDIR) $(BINDIR_ROOT)\YoriInit.d

$(SYMDIR_ROOT):
	@-$(MKDIR) $(SYMDIR_ROOT) 

$(MODDIR_ROOT):
	@-$(MKDIR) $(MODDIR_ROOT) 

all.real[dirs target=install]: $(DIRS) $(SHDIRS)

compile:

link[dirs target=link]: $(DIRS) $(SHDIRS)
!ELSE
all.real: writeconfigcache
	@$(CURRENTTIME)
	@$(FOR) %%i in ($(BINDIR_ROOT) $(SYMDIR_ROOT) $(MODDIR_ROOT) $(BINDIR_ROOT)\YoriInit.d) do $(STARTCMD)@if not exist %%i $(MKDIR) %%i$(STARTCMD)
	@$(FOR) %%i in ($(SHDIRS) $(DIRS)) do $(STARTCMD)@if exist %%i echo *** Compiling %%i & cd %%i & $(BUILD) compile READCONFIGCACHEFILE=..\$(WRITECONFIGCACHEFILE) & cd ..$(STARTCMD)
	@$(FOR) %%i in ($(DIRS)) do $(STARTCMD)@if exist %%i echo *** Linking %%i & cd %%i & $(BUILD) link READCONFIGCACHEFILE=..\$(WRITECONFIGCACHEFILE) & cd ..$(STARTCMD)
	@$(FOR) %%i in ($(SHDIRS)) do $(STARTCMD)@if exist %%i echo *** Linking %%i & cd %%i & $(BUILD) link READCONFIGCACHEFILE=..\$(WRITECONFIGCACHEFILE) & cd ..$(STARTCMD)
	@$(FOR) %%i in ($(SHDIRS) $(DIRS)) do $(STARTCMD)@if exist %%i echo *** Installing %%i & cd %%i & $(BUILD) install READCONFIGCACHEFILE=..\$(WRITECONFIGCACHEFILE) & cd ..$(STARTCMD)
	@$(CURRENTTIME)
!ENDIF

beta: all.real
	@if not exist beta $(MKDIR) beta
	@move $(BINDIR_ROOT) beta\$(ARCH)
	@move $(SYMDIR_ROOT) beta\$(ARCH)\sym

!IFDEF _YMAKE_VER
clean[dirs target=clean]: $(DIRS) $(SHDIRS)
!ELSE
clean: writeconfigcache
	@$(FOR) %%i in ($(SHDIRS) $(DIRS)) do $(STARTCMD)@if exist %%i echo *** Cleaning %%i & cd %%i & $(BUILD) clean READCONFIGCACHEFILE=..\$(WRITECONFIGCACHEFILE) & cd ..$(STARTCMD)
	@if exist *~ erase *~
	@$(FOR_ST) /D %%i in ($(MODDIR_ROOT) $(BINDIR_ROOT) $(SYMDIR_ROOT)) do @if exist %%i $(RMDIR) /s/q %%i
	@if exist $(WRITECONFIGCACHEFILE) erase $(WRITECONFIGCACHEFILE)
!ENDIF

distclean: clean
	@$(FOR_ST) /D %%i in (pkg\*) do @if exist %%i $(RMDIR) /s/q %%i
	@$(FOR_ST) %%i in (beta doc bin sym) do @if exist %%i $(RMDIR) /s/q %%i

help:
	@echo "ANALYZE=[0|1] - If set, will perform static analysis during compilation"
	@echo "DEBUG=[0|1] - If set, will compile debug build without optimization and with instrumentation"
	@echo "PDB=[0|1] - If set, will generate debug symbols"

