/**
 * @file ysetup/helper.c
 *
 * Yori shell installer routines that run in multiple UIs
 *
 * Copyright (c) 2018-2021 Malcolm J. Smith
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <yoripch.h>
#include <yorilib.h>
#include <yoripkg.h>
#include "resource.h"
#include "ysetup.h"

/**
 Error message to display if the system doesn't have Cabinet.dll functions.
 This happens on a custom compile that's not linked against Fdi.lib.
 */
CONST CHAR SetupDllMissingWarning1[] = "Ysetup requires Cabinet.dll and WinInet.dll.\n\n"
                                       "These are included with Internet Explorer 5 or later.\n\n"
                                       "At a minimum, WinInet.dll from Internet Explorer 3 and Cabinet.dll from Internet Explorer 5 can be copied to the System32 directory to proceed.";

/**
 Error message to display if the system doesn't have WinInet.dll functions but
 Cabinet functions are present.  This is the common case in official
 distributions which are statically linked to Cabinet functions.
 */
CONST CHAR SetupDllMissingWarning2[] = "Ysetup requires WinInet.dll.\n\n"
                                       "This is included with Internet Explorer 3 or later.\n\n"
                                       "Alternatively, this can be installed standalone.  See links at http://www.malsmith.net/yori/nt3x/";


/**
 The first block of text to include in any Windows Terminal profile.
 */
CONST CHAR SetupTerminalProfilePart1[] = 
"{\n"
"  \"profiles\": [\n"
"    {\n"
"      \"name\": \"Yori\",\n"
"      \"commandline\": \"";

/**
 The second block of text to include in any Windows Terminal profile.
 */
CONST CHAR SetupTerminalProfilePart2[] = 
"\",\n"
"      \"icon\": \"";

/**
 The third block of text to include in any Windows Terminal profile.
 */
CONST CHAR SetupTerminalProfilePart3[] = 
"\",\n"
"      \"fontFace\": \"Consolas\",\n"
"      \"fontSize\": 10,\n"
"      \"colorScheme\": \"CGA\"\n"
"    }\n"
"  ],\n"
"  \"schemes\": [\n"
"    {\n"
"      \"name\": \"CGA\",\n"
"\n"
"      \"background\": \"#000000\",\n"
"      \"foreground\": \"#AAAAAA\",\n"
"\n"
"      \"black\": \"#000000\",\n"
"      \"red\": \"#AA0000\",\n"
"      \"green\": \"#00AA00\",\n"
"      \"yellow\": \"#AA5500\",\n"
"      \"blue\": \"#0000AA\",\n"
"      \"purple\": \"#AA00AA\",\n"
"      \"cyan\": \"#00AAAA\",\n"
"      \"white\": \"#AAAAAA\",\n"
"      \"brightBlack\": \"#555555\",\n"
"      \"brightRed\": \"#FF5555\",\n"
"      \"brightGreen\": \"#55FF55\",\n"
"      \"brightYellow\": \"#FFFF55\",\n"
"      \"brightBlue\": \"#5555FF\",\n"
"      \"brightPurple\": \"#FF55FF\",\n"
"      \"brightCyan\": \"#55FFFF\",\n"
"      \"brightWhite\": \"#FFFFFF\"\n"
"    }\n"
"  ]\n"
"}\n";

/**
 A list of subdirectories from the application to check for packages.
 */
CONST LPTSTR SetupLocalPathsToCheck[] = {
    _T("pkg"),
    _T("yori"),
    _T("ypm"),
    _T("ysetup")
};

/**
 Return a constant string to display to the user if a required DLL is not
 found.  The message indicates which DLLs are missing.

 @return A string to display to the user.
 */
LPCSTR
SetupGetDllMissingMessage(VOID)
{
    if (DllCabinet.pFdiCopy == NULL) {
        return SetupDllMissingWarning1;
    }
    return SetupDllMissingWarning2;
}

/**
 See if there are local packages for installation without using the internet.
 This probes subdirectories of the application's directory.  Subdirectories
 are used to avoid squatting in the downloads folder, ie., not allowing a
 website drive-by to populate the downloads folder with a file that would
 manipulate the installer.

 @param LocalPath On successful completion, populated with a referenced string
        describing the local package location to use.

 @return TRUE to indicate that a local package location has been found which
         should be used, and FALSE to indicate default package locations
         should be used instead.
 */
__success(return)
BOOL
SetupFindLocalPkgPath(
    __out PYORI_STRING LocalPath
    )
{
    DWORD Index;
    int Result;
    YORI_STRING RelativePathToProbe;
    YORI_STRING FullPathToProbe;

    YoriLibInitEmptyString(&RelativePathToProbe);
    YoriLibInitEmptyString(&FullPathToProbe);
    for (Index = 0; Index < sizeof(SetupLocalPathsToCheck)/sizeof(SetupLocalPathsToCheck[0]); Index++) {

        //
        //  Create a path using the specification for the directory of the
        //  running process
        //

        Result = YoriLibYPrintf(&RelativePathToProbe, _T("~APPDIR\\%s\\pkglist.ini"), SetupLocalPathsToCheck[Index]);
        if (Result == -1) {
            YoriLibFreeStringContents(&RelativePathToProbe);
            YoriLibFreeStringContents(&FullPathToProbe);
            return FALSE;
        }

        //
        //  Turn that into a full path for the benefit of Win32
        //

        if (!YoriLibUserStringToSingleFilePath(&RelativePathToProbe, TRUE, &FullPathToProbe)) {
            YoriLibFreeStringContents(&RelativePathToProbe);
            YoriLibInitEmptyString(&FullPathToProbe);
            return FALSE;
        }

        //
        //  See if it exists
        //

        if (GetFileAttributes(FullPathToProbe.StartOfString) != (DWORD)-1) {
            YoriLibFreeStringContents(&RelativePathToProbe);

            //
            //  Remove pkglist.ini.  Note that sizeof includes the NULL so
            //  this removes the trailing slash too
            //

            FullPathToProbe.LengthInChars -= sizeof("pkglist.ini");
            FullPathToProbe.StartOfString[FullPathToProbe.LengthInChars] = '\0';
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Found local packages at %y\n"), &FullPathToProbe);
            memcpy(LocalPath, &FullPathToProbe, sizeof(YORI_STRING));
            return TRUE;
        }

        YoriLibFreeStringContents(&FullPathToProbe);

    }

    //
    //  No local path found, try the internet
    //

    YoriLibFreeStringContents(&RelativePathToProbe);
    YoriLibFreeStringContents(&FullPathToProbe);
    return FALSE;
}

/**
 Return TRUE if the platform supports shortcuts.  This implies a version 4 OS
 that also has an updated Shell32 or Shfolder to locate the Desktop or Start
 Menu folders.  Technically this could support querying the registry for
 these, but it seems easier to just fall back to DDE rather than have an
 extra case.

 @return TRUE if the platform can create shortcut files, FALSE if it requires
         DDE.
 */
BOOL
SetupPlatformSupportsShortcuts(VOID)
{
    //
    //  If there's no COM, there's no way to create shortcuts.
    //

    if (DllOle32.pCoCreateInstance == NULL ||
        DllOle32.pCoInitialize == NULL) {

        return FALSE;
    }

    //
    //  If there's no SHGetSpecialFolderPath or SHGetFolderPath, we don't
    //  know where to create shortcuts.
    //

    if (DllShell32.pSHGetSpecialFolderPathW == NULL &&
        DllShfolder.pSHGetFolderPathW == NULL) {

        return FALSE;
    }

    return TRUE;
}

/**
 Create a Windows Terminal fragment file adding a Yori profile.

 @param ProfileFileName Pointer to the fully qualified path for the profile
        fragment JSON file.

 @param YoriExeFullPath Pointer to the Yori executable path.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SetupWriteTerminalProfile(
    __in PYORI_STRING ProfileFileName,
    __in PYORI_STRING YoriExeFullPath
    )
{
    HANDLE JsonFile;
    DWORD Index;
    YORI_STRING ParentDirectory;
    DWORD BytesNeeded;
    DWORD BytesWritten;
    YORI_STRING EscapedExePath;
    LPSTR MultibyteEscapedExePath;

    //
    //  Find the parent directory and attempt to create it.
    //

    YoriLibInitEmptyString(&ParentDirectory);
    ParentDirectory.StartOfString = ProfileFileName->StartOfString;
    ParentDirectory.LengthInChars = ProfileFileName->LengthInChars;

    for (Index = ParentDirectory.LengthInChars;
         Index > 0;
         Index--) {

        if (YoriLibIsSep(ParentDirectory.StartOfString[Index - 1])) {
            ParentDirectory.LengthInChars = Index - 1;
            ParentDirectory.StartOfString[Index - 1] = '\0';
            YoriLibCreateDirectoryAndParents(&ParentDirectory);
            ParentDirectory.StartOfString[Index - 1] = '\\';
            break;
        }
    }

    //
    //  Create the JSON file.
    //

    JsonFile = CreateFile(ProfileFileName->StartOfString, GENERIC_WRITE, FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (JsonFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    WriteFile(JsonFile, SetupTerminalProfilePart1, sizeof(SetupTerminalProfilePart1) - 1, &BytesWritten, NULL);

    //
    //  Escape all of the backslashes in the executable path.
    //

    BytesNeeded = 0;
    for (Index = 0; Index < YoriExeFullPath->LengthInChars; Index++) {
        if (YoriExeFullPath->StartOfString[Index] == '\\') {
            BytesNeeded++;
        }
    }

    if (!YoriLibAllocateString(&EscapedExePath, YoriExeFullPath->LengthInChars + BytesNeeded + 1)) {
        CloseHandle(JsonFile);
        DeleteFile(ProfileFileName->StartOfString);
        return FALSE;
    }

    BytesNeeded = 0;
    for (Index = 0; Index < YoriExeFullPath->LengthInChars; Index++) {
        EscapedExePath.StartOfString[Index + BytesNeeded] = YoriExeFullPath->StartOfString[Index];
        if (YoriExeFullPath->StartOfString[Index] == '\\') {
            BytesNeeded++;
            EscapedExePath.StartOfString[Index + BytesNeeded] = YoriExeFullPath->StartOfString[Index];
        }
    }
    EscapedExePath.StartOfString[Index + BytesNeeded] = '\0';
    EscapedExePath.LengthInChars = YoriExeFullPath->LengthInChars + BytesNeeded;

    //
    //  Convert that into UTF-8 for the file contents
    //

    BytesNeeded = YoriLibGetMultibyteOutputSizeNeeded(EscapedExePath.StartOfString, EscapedExePath.LengthInChars);
    MultibyteEscapedExePath = YoriLibMalloc(BytesNeeded);
    if (MultibyteEscapedExePath == NULL) {
        YoriLibFreeStringContents(&EscapedExePath);
        CloseHandle(JsonFile);
        DeleteFile(ProfileFileName->StartOfString);
        return FALSE;
    }

    //
    //  Write the path to the executable
    //

    YoriLibMultibyteOutput(EscapedExePath.StartOfString, EscapedExePath.LengthInChars, MultibyteEscapedExePath, BytesNeeded);
    WriteFile(JsonFile, MultibyteEscapedExePath, BytesNeeded, &BytesWritten, NULL);

    //
    //  Munge it into the path to the icon
    //

    if (BytesNeeded > 3) {
        MultibyteEscapedExePath[BytesNeeded - 3] = 'i';
        MultibyteEscapedExePath[BytesNeeded - 2] = 'c';
        MultibyteEscapedExePath[BytesNeeded - 1] = 'o';
    }
    WriteFile(JsonFile, SetupTerminalProfilePart2, sizeof(SetupTerminalProfilePart2) - 1, &BytesWritten, NULL);
    WriteFile(JsonFile, MultibyteEscapedExePath, BytesNeeded, &BytesWritten, NULL);

    WriteFile(JsonFile, SetupTerminalProfilePart3, sizeof(SetupTerminalProfilePart3) - 1, &BytesWritten, NULL);

    YoriLibFreeStringContents(&EscapedExePath);
    YoriLibFree(MultibyteEscapedExePath);
    CloseHandle(JsonFile);
    return TRUE;
}

/**
 The default application install directory, under Program Files.
 */
#define SETUP_APP_DIR "\\Yori"

/**
 The default application install directory, under Program Files, as Unicode.
 */
#define TSETUP_APP_DIR _T("\\Yori")

#if defined(_MSC_VER) && (_MSC_VER >= 1500)
#pragma warning(push)
#pragma warning(disable: 6260) // sizeof*sizeof is used for character size
                               // flexibility
#endif

/**
 Return the default installation directory.  This is normally
 "C:\Program Files\Yori" but the path can be reconfigured.

 @param InstallDir On successful completion, populated with the default
        install path.

 @return TRUE to indicate successful completion, FALSE on error.
 */
BOOL
SetupGetDefaultInstallDir(
    __out PYORI_STRING InstallDir
    )
{
    HKEY hKey;
    DWORD SizeNeeded;
    DWORD RegType;
    DWORD OsVerMajor;
    DWORD OsVerMinor;
    DWORD OsBuildNumber;
    BOOL IsAdmin;
    YORI_STRING Administrators;

    hKey = NULL;

    YoriLibLoadAdvApi32Functions();
    if (DllAdvApi32.pRegOpenKeyExW == NULL ||
        DllAdvApi32.pRegQueryValueExW == NULL ||
        DllAdvApi32.pRegCloseKey == NULL) {

        goto ReturnDefault;
    }

    //
    //  If the OS provides support for querying group membership (Windows
    //  2000 and above) and the user is not an administrator, default to
    //  a per user installation location.  Otherwise default to a system
    //  location.  Note that local app data didn't exist prior to Windows
    //  2000.
    //

    YoriLibConstantString(&Administrators, _T("Administrators"));
    if (YoriLibIsCurrentUserInGroup(&Administrators, &IsAdmin) && !IsAdmin) {
        YORI_STRING PerUserDir;
        YoriLibConstantString(&PerUserDir, _T("~LOCALAPPDATA\\Yori"));
        if (YoriLibUserStringToSingleFilePath(&PerUserDir, FALSE, InstallDir)) {
            return TRUE;
        }
    }

    //
    //  Query the registry for the location of Program Files.  Because this
    //  installer will download a 64 bit version for a 64 bit system, it wants
    //  to find the "native" program files as opposed to the emulated x86 one.
    //  This registry entry won't exist on 32 bit systems, so it falls back to
    //  the normal program files after that.
    //

    if (DllAdvApi32.pRegOpenKeyExW(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        goto ReturnDefault;
    }

    if (DllAdvApi32.pRegQueryValueExW(hKey, _T("ProgramW6432Dir"), NULL, NULL, NULL, &SizeNeeded) == ERROR_SUCCESS) {
        if (!YoriLibAllocateString(InstallDir, SizeNeeded/sizeof(TCHAR) + sizeof(SETUP_APP_DIR))) {
            goto ReturnDefault;
        }

        SizeNeeded = InstallDir->LengthAllocated * sizeof(TCHAR);
        if (DllAdvApi32.pRegQueryValueExW(hKey, _T("ProgramW6432Dir"), NULL, &RegType, (LPBYTE)InstallDir->StartOfString, &SizeNeeded) == ERROR_SUCCESS) {
            if (RegType != REG_SZ && RegType != REG_EXPAND_SZ) {
                goto ReturnDefault;
            }
            InstallDir->LengthInChars = SizeNeeded / sizeof(TCHAR) - 1;
            if (InstallDir->LengthInChars + sizeof(SETUP_APP_DIR) <= InstallDir->LengthAllocated) {
                memcpy(&InstallDir->StartOfString[InstallDir->LengthInChars], TSETUP_APP_DIR, sizeof(SETUP_APP_DIR) * sizeof(TCHAR));
                InstallDir->LengthInChars += sizeof(SETUP_APP_DIR) - 1;
            } else {
                InstallDir->StartOfString[InstallDir->LengthInChars] = '\0';
            }
        }
        DllAdvApi32.pRegCloseKey(hKey);
        return TRUE;
    }

    if (DllAdvApi32.pRegQueryValueExW(hKey, _T("ProgramFilesDir"), NULL, NULL, NULL, &SizeNeeded) == ERROR_SUCCESS) {
        if (!YoriLibAllocateString(InstallDir, SizeNeeded/sizeof(TCHAR) + sizeof("\\Yori"))) {
            goto ReturnDefault;
        }

        SizeNeeded = InstallDir->LengthAllocated * sizeof(TCHAR);
        if (DllAdvApi32.pRegQueryValueExW(hKey, _T("ProgramFilesDir"), NULL, &RegType, (LPBYTE)InstallDir->StartOfString, &SizeNeeded) == ERROR_SUCCESS) {
            if (RegType != REG_SZ && RegType != REG_EXPAND_SZ) {
                goto ReturnDefault;
            }
            InstallDir->LengthInChars = SizeNeeded / sizeof(TCHAR) - 1;
            if (InstallDir->LengthInChars + sizeof(SETUP_APP_DIR) <= InstallDir->LengthAllocated) {
                memcpy(&InstallDir->StartOfString[InstallDir->LengthInChars], TSETUP_APP_DIR, sizeof(SETUP_APP_DIR) * sizeof(TCHAR));
                InstallDir->LengthInChars += sizeof(SETUP_APP_DIR) - 1;
            } else {
                InstallDir->StartOfString[InstallDir->LengthInChars] = '\0';
            }
        }
        DllAdvApi32.pRegCloseKey(hKey);
        return TRUE;
    }

ReturnDefault:
    YoriLibGetOsVersion(&OsVerMajor, &OsVerMinor, &OsBuildNumber);
    {
        TCHAR WindowsDirectory[MAX_PATH];
        DWORD CharsCopied;

        CharsCopied = GetWindowsDirectory(WindowsDirectory, sizeof(WindowsDirectory)/sizeof(WindowsDirectory[0]));
        if (CharsCopied < 2) {
            YoriLibSPrintf(WindowsDirectory, _T("C:"));
        } else {
            WindowsDirectory[2] = '\0';
        }

        YoriLibInitEmptyString(InstallDir);
        if (OsVerMajor < 4) {
            YoriLibYPrintf(InstallDir, _T("%s\\WIN32APP") TSETUP_APP_DIR, WindowsDirectory);
        } else {
            YoriLibYPrintf(InstallDir, _T("%s\\Program Files") TSETUP_APP_DIR, WindowsDirectory);
        }

        if (InstallDir->StartOfString == NULL) {
            YoriLibConstantString(InstallDir, _T("C:\\Program Files") TSETUP_APP_DIR);
        }
    }
    if (hKey != NULL) {
        DllAdvApi32.pRegCloseKey(hKey);
    }
    return TRUE;
}

#if defined(_MSC_VER) && (_MSC_VER >= 1500)
#pragma warning(pop)
#endif

/**
 Install the user specified set of packages and options.

 @param InstallDir Specifies the target installation directory.

 @param InstallType Specifies whether this is a core, typical, or complete
        install.

 @param InstallOptions Specifies any additional tasks to undertake.

 @param StatusCallback Specifies a callback function to invoke as progress is
        made.

 @param StatusContext Specifies an opaque pointer to pass to StatusCallback.

 @param ErrorText On completion, whether success or failure, populated with a
        string indicating the reason for failure or a success message.  This
        should be freed by the caller, although it will typically not be a
        dynamic allocation.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
SetupInstallSelectedWithOptions(
    __in PYORI_STRING InstallDir,
    __in YSETUP_INSTALL_TYPE InstallType,
    __in DWORD InstallOptions,
    __in PYSETUP_STATUS_CALLBACK StatusCallback,
    __in_opt PVOID StatusContext,
    __out _On_failure_(_Post_valid_) PYORI_STRING ErrorText
    )
{
    YORI_STRING StatusText;
    PYORI_STRING PkgNames;
    PYORI_STRING PackageUrls;
    YORI_STRING LocalPath;
    YORI_STRING ShortcutNameFullPath[3];
    DWORD ShortcutCount;
    PYORI_STRING CustomSource;
    DWORD PkgCount;
    DWORD PkgIndex;
    DWORD PkgUrlCount;
    BOOL Result = FALSE;
    HANDLE OriginalStdOut;
    HANDLE OriginalStdErr;
    HANDLE NulDevice;

    YoriLibInitEmptyString(&LocalPath);
    CustomSource = NULL;
    ShortcutCount = 0;

    OriginalStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    OriginalStdErr = GetStdHandle(STD_ERROR_HANDLE);
    NulDevice = INVALID_HANDLE_VALUE;

    //
    //  Truncate trailing seperators
    //

    while (InstallDir->LengthInChars > 0 &&
           YoriLibIsSep(InstallDir->StartOfString[InstallDir->LengthInChars - 1])) {

        InstallDir->StartOfString[InstallDir->LengthInChars - 1] = '\0';
        InstallDir->LengthInChars--;
    }

    if (InstallDir->LengthInChars == 0) {
        YoriLibConstantString(ErrorText, _T("Installation failed."));
        return FALSE;
    }

    if (!YoriLibCreateDirectoryAndParents(InstallDir)) {
        YoriLibConstantString(ErrorText, _T("Failed to create installation directory.  If installing into a system location, you may want to run the installer as Administrator."));
        return FALSE;
    }

    //
    //  Count the number of packages we want to install
    //

    if (InstallType == InstallTypeComplete) {
        PkgCount = 4;
    } else if (InstallType == InstallTypeCore) {
        PkgCount = 2;
    } else {
        PkgCount = 3;
    }

    if ((InstallOptions & YSETUP_INSTALL_SYMBOLS) != 0) {
        PkgCount *= 2;
    }

    PkgCount++;

    if ((InstallOptions & YSETUP_INSTALL_SOURCE) != 0) {
        PkgCount++;
    }

    //
    //  Allocate a package array
    //

    PkgNames = YoriLibMalloc(PkgCount * sizeof(YORI_STRING));
    if (PkgNames == NULL) {
        YoriLibConstantString(ErrorText, _T("Installation failed."));
        return FALSE;
    }

    //
    //  Populate the array with the names of what we want to install
    //

    if ((InstallOptions & YSETUP_INSTALL_SYMBOLS) != 0) {
        YoriLibConstantString(&PkgNames[0], _T("yori-ypm"));
        YoriLibConstantString(&PkgNames[1], _T("yori-ypm-pdb"));
        YoriLibConstantString(&PkgNames[2], _T("yori-core"));
        YoriLibConstantString(&PkgNames[3], _T("yori-core-pdb"));
        PkgIndex = 4;
        if (InstallType >= InstallTypeTypical) {
            YoriLibConstantString(&PkgNames[4], _T("yori-typical"));
            YoriLibConstantString(&PkgNames[5], _T("yori-typical-pdb"));
            PkgIndex = 6;
        }
        if (InstallType >= InstallTypeComplete) {
            YoriLibConstantString(&PkgNames[6], _T("yori-extra"));
            YoriLibConstantString(&PkgNames[7], _T("yori-extra-pdb"));
            PkgIndex = 8;
        }
    } else {
        YoriLibConstantString(&PkgNames[0], _T("yori-ypm"));
        YoriLibConstantString(&PkgNames[1], _T("yori-core"));
        PkgIndex = 2;
        if (InstallType >= InstallTypeTypical) {
            YoriLibConstantString(&PkgNames[2], _T("yori-typical"));
            PkgIndex = 3;
        }
        if (InstallType >= InstallTypeComplete) {
            YoriLibConstantString(&PkgNames[3], _T("yori-extra"));
            PkgIndex = 4;
        }
    }

    YoriLibConstantString(&PkgNames[PkgIndex], _T("yori-completion"));
    PkgIndex++;

    if ((InstallOptions & YSETUP_INSTALL_SOURCE) != 0) {
        YoriLibConstantString(&PkgNames[PkgIndex], _T("yori-source"));
        PkgIndex++;
    }

    //
    //  Open the NUL device and redirect stdout/stderr to it.  This is to
    //  prevent pkglib dumping all over the console.  Ideally pkglib would
    //  be refactored to return error strings so ypm can display verbose
    //  information and ysetup can handle it on a per-UI basis.
    //

    NulDevice = CreateFile(_T("NUL"), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (NulDevice != INVALID_HANDLE_VALUE) {
        SetStdHandle(STD_OUTPUT_HANDLE, NulDevice);
        SetStdHandle(STD_ERROR_HANDLE, NulDevice);
    }

    //
    //  Obtain URLs for the specified packages.
    //

    if (SetupFindLocalPkgPath(&LocalPath)) {
        CustomSource = &LocalPath;
    }
    YoriLibConstantString(&StatusText, _T("Obtaining package URLs..."));
    SetStdHandle(STD_OUTPUT_HANDLE, OriginalStdOut);
    SetStdHandle(STD_ERROR_HANDLE, OriginalStdErr);
    StatusCallback(&StatusText, StatusContext);
    if (NulDevice != INVALID_HANDLE_VALUE) {
        SetStdHandle(STD_OUTPUT_HANDLE, NulDevice);
        SetStdHandle(STD_ERROR_HANDLE, NulDevice);
    }
    PkgUrlCount = YoriPkgGetRemotePackageUrls(PkgNames, PkgIndex, CustomSource, InstallDir, &PackageUrls);

    if (PkgUrlCount != PkgCount) {
        YoriLibConstantString(ErrorText, _T("Could not locate selected package files."));
        goto Exit;
    }

    //
    //  Install the packages
    //

    for (PkgCount = 0; PkgCount < PkgUrlCount; PkgCount++) {
        YoriLibYPrintf(&StatusText, _T("Installing %i of %i: %y"), PkgCount + 1, PkgUrlCount, &PackageUrls[PkgCount]);
        if (StatusText.StartOfString != NULL) {
            SetStdHandle(STD_OUTPUT_HANDLE, OriginalStdOut);
            SetStdHandle(STD_ERROR_HANDLE, OriginalStdErr);
            StatusCallback(&StatusText, StatusContext);
            if (NulDevice != INVALID_HANDLE_VALUE) {
                SetStdHandle(STD_OUTPUT_HANDLE, NulDevice);
                SetStdHandle(STD_ERROR_HANDLE, NulDevice);
            }
        }
        if (!YoriPkgInstallSinglePackage(&PackageUrls[PkgCount], InstallDir)) {
            YoriLibYPrintf(ErrorText, _T("Failed to install %y from %y"), &PkgNames[PkgCount], &PackageUrls[PkgCount]);
            goto Exit;
        }
    }

    YoriLibFreeStringContents(&StatusText);
    YoriLibConstantString(&StatusText, _T("Applying installation options..."));
    SetStdHandle(STD_OUTPUT_HANDLE, OriginalStdOut);
    SetStdHandle(STD_ERROR_HANDLE, OriginalStdErr);
    StatusCallback(&StatusText, StatusContext);
    if (NulDevice != INVALID_HANDLE_VALUE) {
        SetStdHandle(STD_OUTPUT_HANDLE, NulDevice);
        SetStdHandle(STD_ERROR_HANDLE, NulDevice);
    }

    //
    //  Create shortcuts if requested
    //

    if ((InstallOptions & (YSETUP_INSTALL_DESKTOP_SHORTCUT | YSETUP_INSTALL_START_SHORTCUT | YSETUP_INSTALL_TERMINAL_PROFILE)) != 0) {

        YORI_STRING YSetupPkgName;
        YORI_STRING YSetupPkgVersion;
        YORI_STRING YSetupPkgArch;
        YORI_STRING YoriExeFullPath;
        YORI_STRING RelativeShortcutName;
        YORI_STRING Description;

        YoriLibInitEmptyString(&YoriExeFullPath);
        YoriLibConstantString(&Description, _T("Yori"));

        YoriLibYPrintf(&YoriExeFullPath, _T("%y\\yori.exe"), InstallDir);
        if (YoriExeFullPath.StartOfString == NULL) {
            YoriLibConstantString(ErrorText, _T("Installation failed."));
            goto Exit;
        }

        if ((InstallOptions & YSETUP_INSTALL_DESKTOP_SHORTCUT) != 0) {
            YoriLibInitEmptyString(&ShortcutNameFullPath[ShortcutCount]);

            YoriLibConstantString(&RelativeShortcutName, _T("~Desktop\\Yori.lnk"));
            if (!YoriLibUserStringToSingleFilePath(&RelativeShortcutName, TRUE, &ShortcutNameFullPath[ShortcutCount])) {
                YoriLibFreeStringContents(&YoriExeFullPath);
                YoriLibConstantString(ErrorText, _T("Installation failed."));
                goto Exit;
            }
            ShortcutCount++;
            if (!YoriLibCreateShortcut(&ShortcutNameFullPath[ShortcutCount - 1], &YoriExeFullPath, NULL, &Description, NULL, &YoriExeFullPath, 0, 1, (WORD)-1, TRUE, TRUE)) {
                YoriLibFreeStringContents(&YoriExeFullPath);
                YoriLibConstantString(ErrorText, _T("Failed to create desktop shortcut."));
                goto Exit;
            }
        }

        if ((InstallOptions & YSETUP_INSTALL_START_SHORTCUT) != 0) {

            if (SetupPlatformSupportsShortcuts()) {

                YoriLibInitEmptyString(&ShortcutNameFullPath[ShortcutCount]);

                YoriLibConstantString(&RelativeShortcutName, _T("~Programs\\Yori.lnk"));
                if (!YoriLibUserStringToSingleFilePath(&RelativeShortcutName, TRUE, &ShortcutNameFullPath[ShortcutCount])) {
                    YoriLibFreeStringContents(&YoriExeFullPath);
                    YoriLibConstantString(ErrorText, _T("Installation failed."));
                    goto Exit;
                }
                ShortcutCount++;
                if (!YoriLibCreateShortcut(&ShortcutNameFullPath[ShortcutCount - 1], &YoriExeFullPath, NULL, &Description, NULL, &YoriExeFullPath, 0, 1, (WORD)-1, TRUE, TRUE)) {
                    YoriLibFreeStringContents(&YoriExeFullPath);
                    YoriLibConstantString(ErrorText, _T("Failed to create start menu shortcut."));
                    goto Exit;
                }
            } else {
                YORI_STRING ItemName = YORILIB_CONSTANT_STRING(_T("Yori"));
                if (!YoriLibAddProgmanItem(&ItemName, &ItemName, &YoriExeFullPath, NULL, &YoriExeFullPath, 0)) {
                    YoriLibFreeStringContents(&YoriExeFullPath);
                    YoriLibConstantString(ErrorText, _T("Failed to create Program Manager shortcut."));
                    goto Exit;
                }
            }
        }

        if ((InstallOptions & YSETUP_INSTALL_TERMINAL_PROFILE) != 0) {
            YoriLibInitEmptyString(&ShortcutNameFullPath[ShortcutCount]);

            YoriLibConstantString(&RelativeShortcutName, _T("~LocalAppData\\Microsoft\\Windows Terminal\\Fragments\\Yori\\Yori.json"));
            if (!YoriLibUserStringToSingleFilePath(&RelativeShortcutName, TRUE, &ShortcutNameFullPath[ShortcutCount])) {
                YoriLibFreeStringContents(&YoriExeFullPath);
                YoriLibConstantString(ErrorText, _T("Installation failed."));
                goto Exit;
            }
            ShortcutCount++;

            SetupWriteTerminalProfile(&ShortcutNameFullPath[ShortcutCount - 1],
                                      &YoriExeFullPath);
        }

        YoriLibConstantString(&YSetupPkgName, _T("ysetup-shortcuts"));
        YoriLibConstantString(&YSetupPkgVersion, _T("latest"));
        YoriLibConstantString(&YSetupPkgArch, _T("noarch"));
        YoriPkgInstallPseudoPackage(&YSetupPkgName, &YSetupPkgVersion, &YSetupPkgArch, ShortcutNameFullPath, ShortcutCount, InstallDir);

        YoriLibFreeStringContents(&YoriExeFullPath);
    }

    //
    //  Update paths if requested
    //

    if ((InstallOptions & (YSETUP_INSTALL_USER_PATH | YSETUP_INSTALL_SYSTEM_PATH)) != 0) {

        BOOL AppendToUserPath = FALSE;
        BOOL AppendToSystemPath = FALSE;

        if ((InstallOptions & YSETUP_INSTALL_USER_PATH) != 0) {
            AppendToUserPath = TRUE;
        }

        if ((InstallOptions & YSETUP_INSTALL_SYSTEM_PATH) != 0) {
            AppendToSystemPath = TRUE;
        }

        YoriPkgAppendInstallDirToPath(InstallDir, AppendToUserPath, AppendToSystemPath);
    }

    //
    //  Try to add an uninstall entry
    //

    YoriLibConstantString(ErrorText, _T("Installation complete."));
    SetStdHandle(STD_OUTPUT_HANDLE, OriginalStdOut);
    SetStdHandle(STD_ERROR_HANDLE, OriginalStdErr);
    StatusCallback(ErrorText, StatusContext);
    if (NulDevice != INVALID_HANDLE_VALUE) {
        SetStdHandle(STD_OUTPUT_HANDLE, NulDevice);
        SetStdHandle(STD_ERROR_HANDLE, NulDevice);
    }

    if ((InstallOptions & YSETUP_INSTALL_UNINSTALL) != 0) {

        YORI_STRING VerString;
        TCHAR VerBuffer[32];

#if YORI_BUILD_ID
        YoriLibSPrintf(VerBuffer, _T("%i.%02i.%i"), YORI_VER_MAJOR, YORI_VER_MINOR, YORI_BUILD_ID);
#else
        YoriLibSPrintf(VerBuffer, _T("%i.%02i"), YORI_VER_MAJOR, YORI_VER_MINOR);
#endif

        YoriLibConstantString(&VerString, VerBuffer);

        if (!YoriPkgAddUninstallEntry(InstallDir, &VerString)) {
            YoriLibConstantString(ErrorText, _T("Could not add uninstall entry."));
        }
    }

    Result = TRUE;
Exit:
    for (PkgCount = 0; PkgCount < PkgUrlCount; PkgCount++) {
        YoriLibFreeStringContents(&PackageUrls[PkgCount]);
    }
    for (PkgCount = 0; PkgCount < ShortcutCount; PkgCount++) {
        YoriLibFreeStringContents(&ShortcutNameFullPath[PkgCount]);
    }
    YoriLibDereference(PackageUrls);
    YoriLibFreeStringContents(&StatusText);
    YoriLibFreeStringContents(&LocalPath);
    YoriLibFree(PkgNames);
    SetStdHandle(STD_OUTPUT_HANDLE, OriginalStdOut);
    SetStdHandle(STD_ERROR_HANDLE, OriginalStdErr);
    if (NulDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(NulDevice);
    }
    return Result;
}

// vim:sw=4:ts=4:et:
