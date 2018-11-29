
DEBUG=0
PDB=0
YORI_BUILD_ID=0
ARCH=win32

!IF [$(CC) -? 2>&1 | findstr /C:"for x64" >NUL 2>&1]==0
ARCH=amd64
!ELSE
!IF [$(CC) -? 2>&1 | findstr /C:"for AMD64" >NUL 2>&1]==0
ARCH=amd64
!ELSE
!IF [$(CC) -? 2>&1 | findstr /C:"for ARM64" >NUL 2>&1]==0
ARCH=arm64
!ENDIF
!ENDIF
!ENDIF

BINDIR=bin\$(ARCH)
SYMDIR=sym\$(ARCH)
MODDIR=bin\$(ARCH)\modules

BUILD=$(MAKE) -nologo DEBUG=$(DEBUG) PDB=$(PDB) YORI_BUILD_ID=$(YORI_BUILD_ID) BINDIR=..\$(BINDIR) SYMDIR=..\$(SYMDIR) MODDIR=..\$(MODDIR)

CURRENTTIME=REM
WRITECONFIGCACHEFILE=cache.mk

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
     builtins  \
     cab       \
     cal       \
     clip      \
     clmp      \
     cls       \
     compact   \
     copy      \
     cshot     \
     cut       \
     cvtvt     \
     date      \
     dir       \
     echo      \
     erase     \
     expr      \
     finfo     \
     for       \
     fscmp     \
     get       \
     grpcmp    \
     help      \
	 hexdump   \
     hilite    \
     iconv     \
     initool   \
     intcmp    \
     lines     \
     lsof      \
     mkdir     \
     mklink    \
     more      \
     mount     \
     move      \
     nice      \
     osver     \
     path      \
     pause     \
     pkglib    \
     readline  \
     repl      \
     rmdir     \
     scut      \
     sdir      \
     setver    \
     sleep     \
     split     \
     start     \
     strcmp    \
     sync      \
     tail      \
     tee       \
     title     \
     touch     \
     type      \
     vol       \
     which     \
     wininfo   \
     winpos    \
     ydbg      \
     ypm       \
     ysetup    \

all.real: writeconfigcache
	@$(CURRENTTIME)
	@$(FOR) %%i in ($(BINDIR) $(SYMDIR) $(MODDIR)) do $(STARTCMD)@if not exist %%i $(MKDIR) %%i$(STARTCMD)
	@$(FOR) %%i in ($(SHDIRS) $(DIRS)) do $(STARTCMD)@if exist %%i echo *** Compiling %%i & cd %%i & $(BUILD) compile READCONFIGCACHEFILE=..\$(WRITECONFIGCACHEFILE) & cd ..$(STARTCMD)
	@$(FOR) %%i in ($(DIRS)) do $(STARTCMD)@if exist %%i echo *** Linking %%i & cd %%i & $(BUILD) link READCONFIGCACHEFILE=..\$(WRITECONFIGCACHEFILE) & cd ..$(STARTCMD)
	@$(FOR) %%i in ($(SHDIRS)) do $(STARTCMD)@if exist %%i echo *** Linking %%i & cd %%i & $(BUILD) link READCONFIGCACHEFILE=..\$(WRITECONFIGCACHEFILE) & cd ..$(STARTCMD)
	@$(FOR) %%i in ($(SHDIRS) $(DIRS)) do $(STARTCMD)@if exist %%i echo *** Installing %%i & cd %%i & $(BUILD) install READCONFIGCACHEFILE=..\$(WRITECONFIGCACHEFILE) & cd ..$(STARTCMD)
	@$(CURRENTTIME)

beta: all.real
	@if not exist beta $(MKDIR) beta
	@move $(BINDIR) beta\$(ARCH)
	@move $(SYMDIR) beta\$(ARCH)\sym

clean:
	@$(FOR) %%i in ($(SHDIRS) $(DIRS)) do $(STARTCMD)@if exist %%i echo *** Cleaning %%i & cd %%i & $(BUILD) PROBECOMPILER=0 PROBELINKER=0 clean & cd ..$(STARTCMD)
	@if exist *~ erase *~
	@$(FOR_ST) /D %%i in ($(MODDIR) $(BINDIR) $(SYMDIR)) do @if exist %%i $(RMDIR) /s/q %%i
	@if exist $(WRITECONFIGCACHEFILE) erase $(WRITECONFIGCACHEFILE)

distclean: clean
	@$(FOR_ST) /D %%i in (pkg\*) do @if exist %%i $(RMDIR) /s/q %%i
	@$(FOR_ST) %%i in (beta doc bin sym) do @if exist %%i $(RMDIR) /s/q %%i

help:
	@echo "DEBUG=[0|1] - If set, will compile debug build without optimization and with instrumentation"
	@echo "PDB=[0|1] - If set, will generate debug symbols"

