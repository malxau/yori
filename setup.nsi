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
  File "bin\${PACKARCH}\yori.exe"
  CreateShortCut "$SMPROGRAMS\Yori.lnk" "$INSTDIR\yori.exe" "" "$INSTDIR\yori.exe" 0
SectionEnd
!ENDIF

!IFDEF SHIPPDB
Section "Monolithic Debugging Support"
  SetOutPath "$INSTDIR"
  File "sym\${PACKARCH}\yori.pdb"
SectionEnd
!ENDIF

!IFDEF DBG
!IFNDEF SYMONLY
Section /o "Modular"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\yorimin.exe"
  File "bin\${PACKARCH}\modules\alias.com"
  File "bin\${PACKARCH}\modules\chdir.com"
  File "bin\${PACKARCH}\modules\color.com"
  File "bin\${PACKARCH}\modules\exit.com"
  File "bin\${PACKARCH}\modules\false.com"
  File "bin\${PACKARCH}\modules\fg.com"
  File "bin\${PACKARCH}\modules\if.com"
  File "bin\${PACKARCH}\modules\job.com"
  File "bin\${PACKARCH}\modules\pushd.com"
  File "bin\${PACKARCH}\modules\rem.com"
  File "bin\${PACKARCH}\modules\set.com"
  File "bin\${PACKARCH}\modules\setlocal.com"
  File "bin\${PACKARCH}\modules\true.com"
  File "bin\${PACKARCH}\modules\ver.com"
  File "bin\${PACKARCH}\modules\wait.com"
  File "bin\${PACKARCH}\modules\ys.com"
  CreateShortCut "$SMPROGRAMS\Yori Modular.lnk" "$INSTDIR\yorimin.exe" "" "$INSTDIR\yorimin.exe" 0
SectionEnd
!ENDIF

!IFDEF SHIPPDB
Section /o "Modular Debugging Support"
  SetOutPath "$INSTDIR"
  File "sym\${PACKARCH}\yorimin.pdb"
  File "sym\${PACKARCH}\alias.pdb"
  File "sym\${PACKARCH}\chdir.pdb"
  File "sym\${PACKARCH}\color.pdb"
  File "sym\${PACKARCH}\exit.pdb"
  File "sym\${PACKARCH}\false.pdb"
  File "sym\${PACKARCH}\fg.pdb"
  File "sym\${PACKARCH}\if.pdb"
  File "sym\${PACKARCH}\job.pdb"
  File "sym\${PACKARCH}\pushd.pdb"
  File "sym\${PACKARCH}\rem.pdb"
  File "sym\${PACKARCH}\set.pdb"
  File "sym\${PACKARCH}\setlocal.pdb"
  File "sym\${PACKARCH}\true.pdb"
  File "sym\${PACKARCH}\ver.pdb"
  File "sym\${PACKARCH}\wait.pdb"
  File "sym\${PACKARCH}\ys.pdb"
SectionEnd
!ENDIF
!ENDIF
SectionGroupEnd

SectionGroup "Conventional commands"

!IFNDEF SYMONLY
Section "cls"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\ycls.exe"
SectionEnd

Section "copy"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\ycopy.exe"
SectionEnd

Section "cut"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\ycut.exe"
SectionEnd

Section "date"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\ydate.exe"
SectionEnd

Section "dir"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\ydir.exe"
SectionEnd

Section "echo"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\yecho.exe"
SectionEnd

Section "erase"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\yerase.exe"
SectionEnd

Section "expr"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\yexpr.exe"
SectionEnd

Section "for"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\yfor.exe"
SectionEnd

Section "fscmp"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\fscmp.exe"
SectionEnd

Section "intcmp"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\intcmp.exe"
SectionEnd

Section "mkdir"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\ymkdir.exe"
SectionEnd

Section "mklink"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\ymklink.exe"
SectionEnd

Section "move"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\ymove.exe"
SectionEnd

Section "path"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\ypath.exe"
SectionEnd

Section "pause"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\ypause.exe"
SectionEnd

Section "rmdir"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\yrmdir.exe"
SectionEnd

Section "sleep"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\sleep.exe"
SectionEnd

Section "split"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\ysplit.exe"
SectionEnd

Section "start"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\ystart.exe"
SectionEnd

Section "strcmp"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\strcmp.exe"
SectionEnd

Section "title"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\ytitle.exe"
SectionEnd

Section "type"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\ytype.exe"
SectionEnd

Section "vol"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\yvol.exe"
SectionEnd
!ENDIF

!IFDEF SHIPPDB
Section "Debugging Support"
  SetOutPath "$INSTDIR"
  File "sym\${PACKARCH}\ycls.pdb"
  File "sym\${PACKARCH}\ycopy.pdb"
  File "sym\${PACKARCH}\ycut.pdb"
  File "sym\${PACKARCH}\ydate.pdb"
  File "sym\${PACKARCH}\ydir.pdb"
  File "sym\${PACKARCH}\yecho.pdb"
  File "sym\${PACKARCH}\yerase.pdb"
  File "sym\${PACKARCH}\yexpr.pdb"
  File "sym\${PACKARCH}\yfor.pdb"
  File "sym\${PACKARCH}\fscmp.pdb"
  File "sym\${PACKARCH}\intcmp.pdb"
  File "sym\${PACKARCH}\ymkdir.pdb"
  File "sym\${PACKARCH}\ymklink.pdb"
  File "sym\${PACKARCH}\ymove.pdb"
  File "sym\${PACKARCH}\ypath.pdb"
  File "sym\${PACKARCH}\ypause.pdb"
  File "sym\${PACKARCH}\yrmdir.pdb"
  File "sym\${PACKARCH}\sleep.pdb"
  File "sym\${PACKARCH}\ysplit.pdb"
  File "sym\${PACKARCH}\ystart.pdb"
  File "sym\${PACKARCH}\strcmp.pdb"
  File "sym\${PACKARCH}\ytitle.pdb"
  File "sym\${PACKARCH}\ytype.pdb"
  File "sym\${PACKARCH}\yvol.pdb"
SectionEnd
!ENDIF
SectionGroupEnd

SectionGroup "Useful commands"

!IFNDEF SYMONLY
Section "clip"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\yclip.exe"
SectionEnd

Section "cshot"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\cshot.exe"
SectionEnd

Section "cvtvt"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\cvtvt.exe"
SectionEnd

Section "get"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\yget.exe"
SectionEnd

Section "grpcmp"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\grpcmp.exe"
SectionEnd

Section "help"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\yhelp.exe"
SectionEnd

Section "hilite"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\hilite.exe"
SectionEnd

Section "iconv"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\iconv.exe"
SectionEnd

Section "lines"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\lines.exe"
SectionEnd

Section "nice"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\nice.exe"
SectionEnd

Section "osver"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\osver.exe"
SectionEnd

Section "readline"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\readline.exe"
SectionEnd

Section "scut"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\scut.exe"
SectionEnd

Section "sdir"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\sdir.exe"
SectionEnd

Section "tail"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\tail.exe"
SectionEnd

Section "tee"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\tee.exe"
SectionEnd

Section "which"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\which.exe"
SectionEnd
!ENDIF

!IFDEF SHIPPDB
Section "Debugging Support"
  SetOutPath "$INSTDIR"
  File "sym\${PACKARCH}\yclip.pdb"
  File "sym\${PACKARCH}\cshot.pdb"
  File "sym\${PACKARCH}\cvtvt.pdb"
  File "sym\${PACKARCH}\yget.pdb"
  File "sym\${PACKARCH}\grpcmp.pdb"
  File "sym\${PACKARCH}\yhelp.pdb"
  File "sym\${PACKARCH}\hilite.pdb"
  File "sym\${PACKARCH}\iconv.pdb"
  File "sym\${PACKARCH}\lines.pdb"
  File "sym\${PACKARCH}\nice.pdb"
  File "sym\${PACKARCH}\osver.pdb"
  File "sym\${PACKARCH}\readline.pdb"
  File "sym\${PACKARCH}\scut.pdb"
  File "sym\${PACKARCH}\sdir.pdb"
  File "sym\${PACKARCH}\tail.pdb"
  File "sym\${PACKARCH}\tee.pdb"
  File "sym\${PACKARCH}\which.pdb"
SectionEnd
!ENDIF

SectionGroupEnd

SectionGroup "Additional commands"

!IFNDEF SYMONLY
Section /o "clmp"
  SetOutPath "$INSTDIR"
  File "bin\${PACKARCH}\clmp.exe"
SectionEnd
!ENDIF

!IFDEF SHIPPDB
Section /o "Debugging Support"
  SetOutPath "$INSTDIR"
  File "sym\${PACKARCH}\clmp.pdb"
SectionEnd
!ENDIF

SectionGroupEnd

Section "Completion scripts"
  SetOutPath "$INSTDIR\completion"
  File "completion\chdir.ys1"
  File "completion\chdir.com.ys1"
  File "completion\nice.exe.ys1"
  File "completion\ymkdir.exe.ys1"
  File "completion\yrmdir.exe.ys1"
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
  Delete $INSTDIR\true.com
  Delete $INSTDIR\true.pdb
  Delete $INSTDIR\ytype.exe
  Delete $INSTDIR\ytype.pdb
  Delete $INSTDIR\yvol.exe
  Delete $INSTDIR\yvol.pdb
  Delete $INSTDIR\which.exe
  Delete $INSTDIR\which.pdb
  Delete $INSTDIR\yori-uninstall.exe

  Delete "$SMPROGRAMS\Yori.lnk"
  Delete "$SMPROGRAMS\Yori Modular.lnk"

  RMDir "$INSTDIR"

SectionEnd
!ENDIF
