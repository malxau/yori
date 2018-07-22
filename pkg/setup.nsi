!INCLUDE "Sections.nsh"
!INCLUDE "x64.nsh"

; The name of the installer
Name "Yori utils"
SetCompressor LZMA
XPStyle on

!define /date BUILDDATE "%Y%m%d"

!IFNDEF PACKARCH
!define PACKARCH "win32"
!ENDIF


!IF "${PACKARCH}" == "amd64"
InstallDir $PROGRAMFILES64\bin
!ELSE IF "${PACKARCH}" == "arm64"
InstallDir $PROGRAMFILES64\bin
!ELSE
InstallDir $PROGRAMFILES32\bin
!ENDIF

!IFDEF DBG
!define DBGSUFFIX "-dbg"
!ELSE
!define DBGSUFFIX ""
!ENDIF

!IFDEF SHIPPDB
!define PDBSUFFIX "-withpdb"
!define SYMONLYSUFFIX ""
!ELSE
!IFDEF SYMONLY
!define SYMONLYSUFFIX "-onlypdb"
!define PDBSUFFIX ""
!define SHIPPDB "1"
!ELSE
!define PDBSUFFIX ""
!define SYMONLYSUFFIX ""
!ENDIF
!ENDIF

; The file to write
OutFile "out\yori-${PACKARCH}-installer-${BUILDDATE}${DBGSUFFIX}${PDBSUFFIX}${SYMONLYSUFFIX}.exe"


; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKCU "Software\yori" "Install_Dir"

;--------------------------------

; Pages

Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

RequestExecutionLevel user

; Disable WOW64 redirection
Section
  ${DisableX64FSRedirection}
SectionEnd

; The stuff to install
SectionGroup "Yori Shells"
!IFNDEF SYMONLY
Section "Monolithic"
  SetOutPath "$INSTDIR"
  File "${PACKARCH}${DBGSUFFIX}\bin\yori.exe"
  CreateShortCut "$SMPROGRAMS\Yori.lnk" "$INSTDIR\yori.exe" "" "$INSTDIR\yori.exe" 0
SectionEnd
!ENDIF

!IFDEF SHIPPDB
Section "Monolithic Debugging Support"
  SetOutPath "$INSTDIR"
  File "${PACKARCH}${DBGSUFFIX}\sym\yori.pdb"
SectionEnd
!ENDIF

!IFDEF DBG
!IFNDEF SYMONLY
Section /o "Modular"
  SetOutPath "$INSTDIR"
  File "${PACKARCH}${DBGSUFFIX}\bin\yorimin.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\modules\alias.com"
  File "${PACKARCH}${DBGSUFFIX}\bin\modules\chdir.com"
  File "${PACKARCH}${DBGSUFFIX}\bin\modules\color.com"
  File "${PACKARCH}${DBGSUFFIX}\bin\modules\exit.com"
  File "${PACKARCH}${DBGSUFFIX}\bin\modules\false.com"
  File "${PACKARCH}${DBGSUFFIX}\bin\modules\fg.com"
  File "${PACKARCH}${DBGSUFFIX}\bin\modules\for.com"
  File "${PACKARCH}${DBGSUFFIX}\bin\modules\history.com"
  File "${PACKARCH}${DBGSUFFIX}\bin\modules\if.com"
  File "${PACKARCH}${DBGSUFFIX}\bin\modules\job.com"
  File "${PACKARCH}${DBGSUFFIX}\bin\modules\pushd.com"
  File "${PACKARCH}${DBGSUFFIX}\bin\modules\rem.com"
  File "${PACKARCH}${DBGSUFFIX}\bin\modules\set.com"
  File "${PACKARCH}${DBGSUFFIX}\bin\modules\setlocal.com"
  File "${PACKARCH}${DBGSUFFIX}\bin\modules\true.com"
  File "${PACKARCH}${DBGSUFFIX}\bin\modules\ver.com"
  File "${PACKARCH}${DBGSUFFIX}\bin\modules\wait.com"
  File "${PACKARCH}${DBGSUFFIX}\bin\modules\ys.com"
  CreateShortCut "$SMPROGRAMS\Yori Modular.lnk" "$INSTDIR\yorimin.exe" "" "$INSTDIR\yorimin.exe" 0
  WriteINIStr "$INSTDIR\packages.ini" "Installed" "yori-modular${DBGSUFFIX}" "${YORI_BUILD_STRING}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-modular${DBGSUFFIX}" "Version" "${YORI_BUILD_STRING}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-modular${DBGSUFFIX}" "Architecture" "${PACKARCH}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-modular${DBGSUFFIX}" "UpgradePath" "${YORI_UPGRADE_PATH}/yori-modular${DBGSUFFIX}-${PACKARCH}.cab"
  WriteINIStr "$INSTDIR\packages.ini" "yori-modular${DBGSUFFIX}" "SourcePath" "${YORI_SOURCE_PATH}/yori-source.cab"
  WriteINIStr "$INSTDIR\packages.ini" "yori-modular${DBGSUFFIX}" "SymbolPath" "${YORI_SOURCE_PATH}/yori-modular${DBGSUFFIX}-pdb-${PACKARCH}.cab"
SectionEnd
!ENDIF

!IFDEF SHIPPDB
Section /o "Modular Debugging Support"
  SetOutPath "$INSTDIR"
  File "${PACKARCH}${DBGSUFFIX}\sym\yorimin.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\alias.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\chdir.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\color.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\exit.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\false.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\fg.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\for.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\history.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\if.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\job.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\pushd.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\rem.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\set.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\setlocal.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\true.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\ver.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\wait.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\ys.pdb"
  WriteINIStr "$INSTDIR\packages.ini" "Installed" "yori-modular${DBGSUFFIX}-pdb" "${YORI_BUILD_STRING}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-modular${DBGSUFFIX}-pdb" "Version" "${YORI_BUILD_STRING}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-modular${DBGSUFFIX}-pdb" "Architecture" "${PACKARCH}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-modular${DBGSUFFIX}-pdb" "UpgradePath" "${YORI_UPGRADE_PATH}/yori-modular${DBGSUFFIX}-pdb-${PACKARCH}.cab"
  WriteINIStr "$INSTDIR\packages.ini" "yori-modular${DBGSUFFIX}-pdb" "SourcePath" "${YORI_SOURCE_PATH}/yori-source.cab"
SectionEnd
!ENDIF
!ENDIF
SectionGroupEnd

!IFNDEF SYMONLY
!IFDEF SHIPPDB
SectionGroup "Conventional commands"
!ENDIF
!ENDIF

!IFNDEF SYMONLY
!IFDEF SHIPPDB
Section
!ELSE
Section "Conventional commands"
!ENDIF
  SetOutPath "$INSTDIR"
  File "${PACKARCH}${DBGSUFFIX}\bin\ycls.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\ycopy.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\ycut.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\ydate.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\ydir.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\yecho.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\yerase.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\yexpr.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\fscmp.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\intcmp.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\ymkdir.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\ymklink.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\ymove.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\ypath.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\ypause.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\yrmdir.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\sleep.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\ysplit.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\ystart.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\strcmp.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\ytitle.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\ytype.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\yvol.exe"
  WriteINIStr "$INSTDIR\packages.ini" "Installed" "yori-core${DBGSUFFIX}" "${YORI_BUILD_STRING}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-core${DBGSUFFIX}" "Version" "${YORI_BUILD_STRING}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-core${DBGSUFFIX}" "Architecture" "${PACKARCH}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-core${DBGSUFFIX}" "UpgradePath" "${YORI_UPGRADE_PATH}/yori-core${DBGSUFFIX}-${PACKARCH}.cab"
  WriteINIStr "$INSTDIR\packages.ini" "yori-core${DBGSUFFIX}" "SourcePath" "${YORI_SOURCE_PATH}/yori-source.cab"
  WriteINIStr "$INSTDIR\packages.ini" "yori-core${DBGSUFFIX}" "SymbolPath" "${YORI_SOURCE_PATH}/yori-core${DBGSUFFIX}-pdb-${PACKARCH}.cab"
SectionEnd
!ENDIF

!IFDEF SHIPPDB
!IFNDEF SYMONLY
Section "Debugging Support"
!ELSE
Section "Conventional commands"
!ENDIF
  SetOutPath "$INSTDIR"
  File "${PACKARCH}${DBGSUFFIX}\sym\ycls.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\ycopy.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\ycut.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\ydate.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\ydir.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\yecho.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\yerase.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\yexpr.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\fscmp.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\intcmp.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\ymkdir.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\ymklink.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\ymove.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\ypath.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\ypause.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\yrmdir.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\sleep.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\ysplit.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\ystart.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\strcmp.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\ytitle.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\ytype.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\yvol.pdb"
  WriteINIStr "$INSTDIR\packages.ini" "Installed" "yori-core${DBGSUFFIX}-pdb" "${YORI_BUILD_STRING}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-core${DBGSUFFIX}-pdb" "Version" "${YORI_BUILD_STRING}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-core${DBGSUFFIX}-pdb" "Architecture" "${PACKARCH}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-core${DBGSUFFIX}-pdb" "UpgradePath" "${YORI_UPGRADE_PATH}/yori-core${DBGSUFFIX}-pdb-${PACKARCH}.cab"
  WriteINIStr "$INSTDIR\packages.ini" "yori-core${DBGSUFFIX}-pdb" "SourcePath" "${YORI_SOURCE_PATH}/yori-source.cab"
SectionEnd
!ENDIF

!IFNDEF SYMONLY
!IFDEF SHIPPDB
SectionGroupEnd
!ENDIF
!ENDIF

!IFNDEF SYMONLY
!IFDEF SHIPPDB
SectionGroup "Useful commands"
!ENDIF
!ENDIF

!IFNDEF SYMONLY
!IFNDEF SHIPPDB
Section "Useful commands"
!ELSE
Section
!ENDIF
  SetOutPath "$INSTDIR"
  File "${PACKARCH}${DBGSUFFIX}\bin\yclip.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\cshot.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\cvtvt.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\yget.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\finfo.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\grpcmp.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\yhelp.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\hilite.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\iconv.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\lines.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\nice.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\osver.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\readline.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\scut.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\sdir.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\setver.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\tail.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\tee.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\touch.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\which.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\ypm.exe"
  WriteINIStr "$INSTDIR\packages.ini" "Installed" "yori-ypm${DBGSUFFIX}" "${YORI_BUILD_STRING}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-ypm${DBGSUFFIX}" "Version" "${YORI_BUILD_STRING}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-ypm${DBGSUFFIX}" "Architecture" "${PACKARCH}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-ypm${DBGSUFFIX}" "UpgradePath" "${YORI_UPGRADE_PATH}/yori-ypm${DBGSUFFIX}-${PACKARCH}.cab"
  WriteINIStr "$INSTDIR\packages.ini" "yori-ypm${DBGSUFFIX}" "SourcePath" "${YORI_SOURCE_PATH}/yori-source.cab"
  WriteINIStr "$INSTDIR\packages.ini" "yori-ypm${DBGSUFFIX}" "SymbolPath" "${YORI_SOURCE_PATH}/yori-ypm${DBGSUFFIX}-pdb-${PACKARCH}.cab"
  WriteINIStr "$INSTDIR\packages.ini" "Installed" "yori-typical${DBGSUFFIX}" "${YORI_BUILD_STRING}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-typical${DBGSUFFIX}" "Version" "${YORI_BUILD_STRING}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-typical${DBGSUFFIX}" "Architecture" "${PACKARCH}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-typical${DBGSUFFIX}" "UpgradePath" "${YORI_UPGRADE_PATH}/yori-typical${DBGSUFFIX}-${PACKARCH}.cab"
  WriteINIStr "$INSTDIR\packages.ini" "yori-typical${DBGSUFFIX}" "SourcePath" "${YORI_SOURCE_PATH}/yori-source.cab"
  WriteINIStr "$INSTDIR\packages.ini" "yori-typical${DBGSUFFIX}" "SymbolPath" "${YORI_SOURCE_PATH}/yori-typical${DBGSUFFIX}-pdb-${PACKARCH}.cab"
SectionEnd
!ENDIF

!IFDEF SHIPPDB
!IFNDEF SYMONLY
Section "Debugging Support"
!ELSE
Section "Useful commands"
!ENDIF
  SetOutPath "$INSTDIR"
  File "${PACKARCH}${DBGSUFFIX}\sym\yclip.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\cshot.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\cvtvt.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\finfo.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\yget.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\grpcmp.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\yhelp.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\hilite.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\iconv.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\lines.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\nice.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\osver.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\readline.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\scut.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\sdir.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\setver.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\tail.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\tee.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\touch.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\which.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\ypm.pdb"
  WriteINIStr "$INSTDIR\packages.ini" "Installed" "yori-ypm${DBGSUFFIX}-pdb" "${YORI_BUILD_STRING}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-ypm${DBGSUFFIX}-pdb" "Version" "${YORI_BUILD_STRING}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-ypm${DBGSUFFIX}-pdb" "Architecture" "${PACKARCH}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-ypm${DBGSUFFIX}-pdb" "UpgradePath" "${YORI_UPGRADE_PATH}/yori-ypm${DBGSUFFIX}-pdb-${PACKARCH}.cab"
  WriteINIStr "$INSTDIR\packages.ini" "yori-ypm${DBGSUFFIX}-pdb" "SourcePath" "${YORI_SOURCE_PATH}/yori-source.cab"
  WriteINIStr "$INSTDIR\packages.ini" "Installed" "yori-typical${DBGSUFFIX}-pdb" "${YORI_BUILD_STRING}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-typical${DBGSUFFIX}-pdb" "Version" "${YORI_BUILD_STRING}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-typical${DBGSUFFIX}-pdb" "Architecture" "${PACKARCH}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-typical${DBGSUFFIX}-pdb" "UpgradePath" "${YORI_UPGRADE_PATH}/yori-typical${DBGSUFFIX}-pdb-${PACKARCH}.cab"
  WriteINIStr "$INSTDIR\packages.ini" "yori-typical${DBGSUFFIX}-pdb" "SourcePath" "${YORI_SOURCE_PATH}/yori-source.cab"
SectionEnd
!ENDIF

!IFNDEF SYMONLY
!IFDEF SHIPPDB
SectionGroupEnd
!ENDIF
!ENDIF

!IFNDEF SYMONLY
!IFDEF SHIPPDB
SectionGroup "Additional commands"
!ENDIF
!ENDIF

!IFNDEF SYMONLY
!IFNDEF SHIPPDB
Section /o "Additional commands"
!ELSE
Section /o "Binaries"
!ENDIF
  SetOutPath "$INSTDIR"
  File "${PACKARCH}${DBGSUFFIX}\bin\clmp.exe"
  File "${PACKARCH}${DBGSUFFIX}\bin\yfor.exe"
  WriteINIStr "$INSTDIR\packages.ini" "Installed" "yori-extra${DBGSUFFIX}" "${YORI_BUILD_STRING}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-extra${DBGSUFFIX}" "Version" "${YORI_BUILD_STRING}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-extra${DBGSUFFIX}" "Architecture" "${PACKARCH}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-extra${DBGSUFFIX}" "UpgradePath" "${YORI_UPGRADE_PATH}/yori-extra${DBGSUFFIX}-${PACKARCH}.cab"
  WriteINIStr "$INSTDIR\packages.ini" "yori-extra${DBGSUFFIX}" "SourcePath" "${YORI_SOURCE_PATH}/yori-source.cab"
  WriteINIStr "$INSTDIR\packages.ini" "yori-extra${DBGSUFFIX}" "SymbolPath" "${YORI_SOURCE_PATH}/yori-extra${DBGSUFFIX}-pdb-${PACKARCH}.cab"
SectionEnd
!ENDIF

!IFDEF SHIPPDB
!IFNDEF SYMONLY
Section /o "Debugging Support"
!ELSE
Section /o "Additional commands"
!ENDIF
  SetOutPath "$INSTDIR"
  File "${PACKARCH}${DBGSUFFIX}\sym\clmp.pdb"
  File "${PACKARCH}${DBGSUFFIX}\sym\yfor.pdb"
  WriteINIStr "$INSTDIR\packages.ini" "Installed" "yori-extra${DBGSUFFIX}-pdb" "${YORI_BUILD_STRING}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-extra${DBGSUFFIX}-pdb" "Version" "${YORI_BUILD_STRING}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-extra${DBGSUFFIX}-pdb" "Architecture" "${PACKARCH}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-extra${DBGSUFFIX}-pdb" "UpgradePath" "${YORI_UPGRADE_PATH}/yori-extra${DBGSUFFIX}-pdb-${PACKARCH}.cab"
  WriteINIStr "$INSTDIR\packages.ini" "yori-extra${DBGSUFFIX}-pdb" "SourcePath" "${YORI_SOURCE_PATH}/yori-source.cab"
SectionEnd
!ENDIF

!IFNDEF SYMONLY
!IFDEF SHIPPDB
SectionGroupEnd
!ENDIF
!ENDIF

Section "Completion scripts"
  SetOutPath "$INSTDIR\completion"
  File "..\completion\chdir.ys1"
  File "..\completion\chdir.com.ys1"
  File "..\completion\nice.exe.ys1"
  File "..\completion\ymkdir.exe.ys1"
  File "..\completion\yrmdir.exe.ys1"
  WriteINIStr "$INSTDIR\packages.ini" "Installed" "yori-completion" "${YORI_BUILD_STRING}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-completion" "Version" "${YORI_BUILD_STRING}"
  WriteINIStr "$INSTDIR\packages.ini" "yori-completion" "Architecture" "noarch"
  WriteINIStr "$INSTDIR\packages.ini" "yori-completion" "UpgradePath" "${YORI_UPGRADE_PATH}/yori-completion-noarch.cab"
  WriteINIStr "$INSTDIR\packages.ini" "yori-completion" "SourcePath" "${YORI_SOURCE_PATH}/yori-source.cab"
SectionEnd

!IFNDEF SYMONLY

Section "Uninstall support"

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  AddSize 50

  ; Install to per-user start menu so we don't require admin privs
  ; Note admin is still required for add/remove programs support
  SetShellVarContext current

  ; Write the installation path into the registry
  WriteRegStr HKLM "SOFTWARE\yori" "Install_Dir" "$INSTDIR"
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\YoriUtils" "DisplayName" "Yori Utils"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\YoriUtils" "UninstallString" '"$INSTDIR\yori-uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\YoriUtils" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\YoriUtils" "NoRepair" 1
  WriteUninstaller "yori-uninstall.exe"

SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"

  ${DisableX64FSRedirection}

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ; Install to per-user start menu so we don't require admin privs
  ; Note admin is still required for add/remove programs support
  SetShellVarContext current

  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\YoriUtils"
  DeleteRegKey HKLM "SOFTWARE\Yori"

  ; Remove files and uninstaller
  Delete $INSTDIR\yori.exe
  Delete $INSTDIR\yori.pdb
!IFDEF DBG
  Delete $INSTDIR\yorimin.exe
  Delete $INSTDIR\yorimin.pdb
  Delete $INSTDIR\alias.com
  Delete $INSTDIR\alias.pdb
  Delete $INSTDIR\chdir.com
  Delete $INSTDIR\chdir.pdb
  Delete $INSTDIR\color.com
  Delete $INSTDIR\color.pdb
  Delete $INSTDIR\exit.com
  Delete $INSTDIR\exit.pdb
  Delete $INSTDIR\false.com
  Delete $INSTDIR\false.pdb
  Delete $INSTDIR\fg.com
  Delete $INSTDIR\fg.pdb
  Delete $INSTDIR\for.com
  Delete $INSTDIR\for.pdb
  Delete $INSTDIR\history.com
  Delete $INSTDIR\history.pdb
  Delete $INSTDIR\if.com
  Delete $INSTDIR\if.pdb
  Delete $INSTDIR\job.com
  Delete $INSTDIR\job.pdb
  Delete $INSTDIR\pushd.com
  Delete $INSTDIR\pushd.pdb
  Delete $INSTDIR\rem.com
  Delete $INSTDIR\rem.pdb
  Delete $INSTDIR\set.com
  Delete $INSTDIR\set.pdb
  Delete $INSTDIR\setlocal.com
  Delete $INSTDIR\setlocal.pdb
  Delete $INSTDIR\ver.com
  Delete $INSTDIR\ver.pdb
  Delete $INSTDIR\wait.com
  Delete $INSTDIR\wait.pdb
  Delete $INSTDIR\ys.com
  Delete $INSTDIR\ys.pdb
!ENDIF
  Delete $INSTDIR\yclip.exe
  Delete $INSTDIR\yclip.pdb
  Delete $INSTDIR\clmp.exe
  Delete $INSTDIR\clmp.pdb
  Delete $INSTDIR\ycls.exe
  Delete $INSTDIR\ycls.pdb
  Delete $INSTDIR\ycopy.exe
  Delete $INSTDIR\ycopy.pdb
  Delete $INSTDIR\cshot.exe
  Delete $INSTDIR\cshot.pdb
  Delete $INSTDIR\ycut.exe
  Delete $INSTDIR\ycut.pdb
  Delete $INSTDIR\cvtvt.exe
  Delete $INSTDIR\cvtvt.pdb
  Delete $INSTDIR\ydate.exe
  Delete $INSTDIR\ydate.pdb
  Delete $INSTDIR\ydir.exe
  Delete $INSTDIR\ydir.pdb
  Delete $INSTDIR\yecho.exe
  Delete $INSTDIR\yecho.pdb
  Delete $INSTDIR\yerase.exe
  Delete $INSTDIR\yerase.pdb
  Delete $INSTDIR\yexpr.exe
  Delete $INSTDIR\yexpr.pdb
  Delete $INSTDIR\finfo.exe
  Delete $INSTDIR\finfo.pdb
  Delete $INSTDIR\yfor.exe
  Delete $INSTDIR\yfor.pdb
  Delete $INSTDIR\fscmp.exe
  Delete $INSTDIR\fscmp.pdb
  Delete $INSTDIR\yget.exe
  Delete $INSTDIR\yget.pdb
  Delete $INSTDIR\grpcmp.exe
  Delete $INSTDIR\grpcmp.pdb
  Delete $INSTDIR\yhelp.exe
  Delete $INSTDIR\yhelp.pdb
  Delete $INSTDIR\hilite.exe
  Delete $INSTDIR\hilite.pdb
  Delete $INSTDIR\iconv.exe
  Delete $INSTDIR\iconv.pdb
  Delete $INSTDIR\intcmp.exe
  Delete $INSTDIR\intcmp.pdb
  Delete $INSTDIR\lines.exe
  Delete $INSTDIR\lines.pdb
  Delete $INSTDIR\ymkdir.exe
  Delete $INSTDIR\ymkdir.pdb
  Delete $INSTDIR\ymklink.exe
  Delete $INSTDIR\ymklink.pdb
  Delete $INSTDIR\ymove.exe
  Delete $INSTDIR\ymove.pdb
  Delete $INSTDIR\nice.exe
  Delete $INSTDIR\nice.pdb
  Delete $INSTDIR\osver.exe
  Delete $INSTDIR\osver.pdb
  Delete $INSTDIR\ypath.exe
  Delete $INSTDIR\ypath.pdb
  Delete $INSTDIR\ypause.exe
  Delete $INSTDIR\ypause.pdb
  Delete $INSTDIR\readline.exe
  Delete $INSTDIR\readline.pdb
  Delete $INSTDIR\yrmdir.exe
  Delete $INSTDIR\yrmdir.pdb
  Delete $INSTDIR\scut.exe
  Delete $INSTDIR\scut.pdb
  Delete $INSTDIR\sdir.exe
  Delete $INSTDIR\sdir.pdb
  Delete $INSTDIR\setver.exe
  Delete $INSTDIR\setver.pdb
  Delete $INSTDIR\sleep.exe
  Delete $INSTDIR\sleep.pdb
  Delete $INSTDIR\ysplit.exe
  Delete $INSTDIR\ysplit.pdb
  Delete $INSTDIR\ystart.exe
  Delete $INSTDIR\ystart.pdb
  Delete $INSTDIR\strcmp.exe
  Delete $INSTDIR\strcmp.pdb
  Delete $INSTDIR\tail.exe
  Delete $INSTDIR\tail.pdb
  Delete $INSTDIR\tee.exe
  Delete $INSTDIR\tee.pdb
  Delete $INSTDIR\ytitle.exe
  Delete $INSTDIR\ytitle.pdb
  Delete $INSTDIR\touch.exe
  Delete $INSTDIR\touch.pdb
  Delete $INSTDIR\true.com
  Delete $INSTDIR\true.pdb
  Delete $INSTDIR\ytype.exe
  Delete $INSTDIR\ytype.pdb
  Delete $INSTDIR\yvol.exe
  Delete $INSTDIR\yvol.pdb
  Delete $INSTDIR\which.exe
  Delete $INSTDIR\which.pdb
  Delete $INSTDIR\ypm.exe
  Delete $INSTDIR\ypm.pdb
  Delete $INSTDIR\yori-uninstall.exe

  Delete "$SMPROGRAMS\Yori.lnk"
  Delete "$SMPROGRAMS\Yori Modular.lnk"

  RMDir "$INSTDIR"

SectionEnd
!ENDIF
