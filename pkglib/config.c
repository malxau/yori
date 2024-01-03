/**
 * @file pkglib/config.c
 *
 * Yori shell package manager routines to install system configuration
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
#include "yoripkg.h"
#include "yoripkgp.h"

/**
 The first block of text to include in any Windows Terminal profile.
 */
CONST CHAR YoriPkgTerminalProfilePart1[] = 
"{\n"
"  \"profiles\": [\n"
"    {\n"
"      \"name\": \"Yori\",\n"
"      \"commandline\": \"";

/**
 The second block of text to include in any Windows Terminal profile.
 */
CONST CHAR YoriPkgTerminalProfilePart2[] = 
"\",\n"
"      \"icon\": \"";

/**
 The third block of text to include in any Windows Terminal profile.
 */
CONST CHAR YoriPkgTerminalProfilePart3[] = 
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
 A color table to use when creating shortcuts.  This table is the CGA table,
 the same one used for Windows Terminal above.
 */
CONST COLORREF YoriPkgCgaColorTable[] = {
    RGB(0x00, 0x00, 0x00),
    RGB(0x00, 0x00, 0xAA),
    RGB(0x00, 0xAA, 0x00),
    RGB(0x00, 0xAA, 0xAA),
    RGB(0xAA, 0x00, 0x00),
    RGB(0xAA, 0x00, 0xAA),
    RGB(0xAA, 0x55, 0x00),
    RGB(0xAA, 0xAA, 0xAA),
    RGB(0x55, 0x55, 0x55),
    RGB(0x55, 0x55, 0xFF),
    RGB(0x55, 0xFF, 0x55),
    RGB(0x55, 0xFF, 0xFF),
    RGB(0xFF, 0x55, 0x55),
    RGB(0xFF, 0x55, 0xFF),
    RGB(0xFF, 0xFF, 0x55),
    RGB(0xFF, 0xFF, 0xFF)
};

/**
 On successful completion, returns the path to the current user's Windows
 Terminal fragment file.

 @param TerminalProfilePath Pointer to a string which will be allocated in
        this routine and updated to contain the path to the Windows Terminal
        fragment file.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriPkgGetTerminalProfilePath(
    __out PYORI_STRING TerminalProfilePath
    )
{
    YORI_STRING RelativePath;
    YORI_STRING FullPath;

    YoriLibConstantString(&RelativePath, _T("~LocalAppData\\Microsoft\\Windows Terminal\\Fragments\\Yori\\Yori.json"));
    YoriLibInitEmptyString(&FullPath);
    if (!YoriLibUserStringToSingleFilePath(&RelativePath, TRUE, &FullPath)) {
        return FALSE;
    }

    memcpy(TerminalProfilePath, &FullPath, sizeof(YORI_STRING));
    return TRUE;
}

/**
 Construct a default path to the Yori.exe executable.  This firstly checks
 the YORISPEC environment variable, and if that's not found, assume it is in
 the same directory as the running process.

 @param YoriExecutablePath On successful completion, updated within this
        routine to point to a full path to the Yori.exe executable.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriPkgGetYoriExecutablePath(
    __out PYORI_STRING YoriExecutablePath
    )
{
    YORI_STRING RelativePath;
    YORI_STRING LocalExePath;
    BOOLEAN UseAppDir;

    YoriLibInitEmptyString(&LocalExePath);
    UseAppDir = FALSE;
    if (!YoriLibAllocateAndGetEnvironmentVariable(_T("YORISPEC"), &LocalExePath)) {
        UseAppDir = TRUE;
    } else if (LocalExePath.LengthInChars == 0) {
        YoriLibFreeStringContents(&LocalExePath);
        UseAppDir = TRUE;
    }

    if (UseAppDir) {
        YoriLibConstantString(&RelativePath, _T("~APPDIR\\Yori.exe"));
        if (!YoriLibUserStringToSingleFilePath(&RelativePath, FALSE, &LocalExePath)) {
            return FALSE;
        }
    }

    memcpy(YoriExecutablePath, &LocalExePath, sizeof(YORI_STRING));
    return TRUE;
}

/**
 Construct a default path to the Yui.exe executable, assuming it is in the
 same directory as the running process.

 @param YoriExecutablePath On successful completion, updated within this
        routine to point to a full path to the Yori.exe executable.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriPkgGetYuiExecutablePath(
    __out PYORI_STRING YoriExecutablePath
    )
{
    YORI_STRING RelativePath;
    YORI_STRING LocalExePath;

    YoriLibConstantString(&RelativePath, _T("~APPDIR\\Yui.exe"));
    YoriLibInitEmptyString(&LocalExePath);
    if (!YoriLibUserStringToSingleFilePath(&RelativePath, FALSE, &LocalExePath)) {
        return FALSE;
    }

    memcpy(YoriExecutablePath, &LocalExePath, sizeof(YORI_STRING));
    return TRUE;
}

/**
 Create a Windows Terminal fragment file adding a Yori profile.

 @param YoriExeFullPath Optional pointer to the Yori executable path.  If not
        specified, a file called "yori.exe" in the same directory as the
        current executable is used.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriPkgWriteTerminalProfile(
    __in_opt PYORI_STRING YoriExeFullPath
    )
{
    HANDLE JsonFile;
    YORI_ALLOC_SIZE_T Index;
    YORI_STRING ParentDirectory;
    YORI_ALLOC_SIZE_T BytesNeeded;
    DWORD BytesWritten;
    YORI_STRING LocalExePath;
    YORI_STRING EscapedExePath;
    YORI_STRING ProfileFileName;
    LPSTR MultibyteEscapedExePath;

    if (!YoriPkgGetTerminalProfilePath(&ProfileFileName)) {
        return FALSE;
    }

    if (YoriExeFullPath == NULL) {
        if (!YoriPkgGetYoriExecutablePath(&LocalExePath)) {
            YoriLibFreeStringContents(&ProfileFileName);
            return FALSE;
        }
    } else {
        YoriLibCloneString(&LocalExePath, YoriExeFullPath);
    }

    //
    //  Find the parent directory and attempt to create it.
    //

    YoriLibInitEmptyString(&ParentDirectory);
    ParentDirectory.StartOfString = ProfileFileName.StartOfString;
    ParentDirectory.LengthInChars = ProfileFileName.LengthInChars;

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

    JsonFile = CreateFile(ProfileFileName.StartOfString, GENERIC_WRITE, FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (JsonFile == INVALID_HANDLE_VALUE) {
        YoriLibFreeStringContents(&LocalExePath);
        YoriLibFreeStringContents(&ProfileFileName);
        return FALSE;
    }

    WriteFile(JsonFile, YoriPkgTerminalProfilePart1, sizeof(YoriPkgTerminalProfilePart1) - 1, &BytesWritten, NULL);

    //
    //  Escape all of the backslashes in the executable path.
    //

    BytesNeeded = 0;
    for (Index = 0; Index < LocalExePath.LengthInChars; Index++) {
        if (LocalExePath.StartOfString[Index] == '\\') {
            BytesNeeded++;
        }
    }

    if (!YoriLibAllocateString(&EscapedExePath, LocalExePath.LengthInChars + BytesNeeded + 1)) {
        CloseHandle(JsonFile);
        DeleteFile(ProfileFileName.StartOfString);
        YoriLibFreeStringContents(&LocalExePath);
        YoriLibFreeStringContents(&ProfileFileName);
        return FALSE;
    }

    BytesNeeded = 0;
    for (Index = 0; Index < LocalExePath.LengthInChars; Index++) {
        EscapedExePath.StartOfString[Index + BytesNeeded] = LocalExePath.StartOfString[Index];
        if (LocalExePath.StartOfString[Index] == '\\') {
            BytesNeeded++;
            EscapedExePath.StartOfString[Index + BytesNeeded] = LocalExePath.StartOfString[Index];
        }
    }
    EscapedExePath.StartOfString[Index + BytesNeeded] = '\0';
    EscapedExePath.LengthInChars = LocalExePath.LengthInChars + BytesNeeded;

    //
    //  Convert that into UTF-8 for the file contents
    //

    BytesNeeded = YoriLibGetMultibyteOutputSizeNeeded(EscapedExePath.StartOfString, EscapedExePath.LengthInChars);
    MultibyteEscapedExePath = YoriLibMalloc(BytesNeeded);
    if (MultibyteEscapedExePath == NULL) {
        YoriLibFreeStringContents(&EscapedExePath);
        CloseHandle(JsonFile);
        DeleteFile(ProfileFileName.StartOfString);
        YoriLibFreeStringContents(&LocalExePath);
        YoriLibFreeStringContents(&ProfileFileName);
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
    WriteFile(JsonFile, YoriPkgTerminalProfilePart2, sizeof(YoriPkgTerminalProfilePart2) - 1, &BytesWritten, NULL);
    WriteFile(JsonFile, MultibyteEscapedExePath, BytesNeeded, &BytesWritten, NULL);

    WriteFile(JsonFile, YoriPkgTerminalProfilePart3, sizeof(YoriPkgTerminalProfilePart3) - 1, &BytesWritten, NULL);

    YoriLibFreeStringContents(&EscapedExePath);
    YoriLibFree(MultibyteEscapedExePath);
    YoriLibFreeStringContents(&LocalExePath);
    YoriLibFreeStringContents(&ProfileFileName);
    CloseHandle(JsonFile);
    return TRUE;
}

/**
 Create a shortcut to a Yori shell.

 @param ShortcutPath Pointer to the path to store the shortcut.  This can
        include Yori specific path specifiers such as ~Desktop.

 @param YoriExeFullPath Optionally points to a string specifying the shell
        executable.  If not supplied, Yori.exe within the directory containing
        the running process is used.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriPkgCreateAppShortcut(
    __in PCYORI_STRING ShortcutPath,
    __in_opt PYORI_STRING YoriExeFullPath
    )
{
    YORI_STRING LocalExePath;
    YORI_STRING FullShortcutPath;
    YORI_STRING WorkingDir;
    YORI_STRING Description;
    YORI_STRING HomeDir;
    PISHELLLINKDATALIST_CONSOLE_PROPS ConsoleProps;

    if (YoriExeFullPath == NULL) {
        if (!YoriPkgGetYoriExecutablePath(&LocalExePath)) {
            return FALSE;
        }
    } else {
        YoriLibCloneString(&LocalExePath, YoriExeFullPath);
    }

    if (!YoriLibUserStringToSingleFilePath(ShortcutPath, TRUE, &FullShortcutPath)) {
        YoriLibFreeStringContents(&LocalExePath);
        return FALSE;
    }

    YoriLibConstantString(&HomeDir, _T("~"));
    if (!YoriLibUserStringToSingleFilePath(&HomeDir, FALSE, &WorkingDir)) {
        YoriLibFreeStringContents(&FullShortcutPath);
        YoriLibFreeStringContents(&LocalExePath);
        return FALSE;
    }

    ConsoleProps = YoriLibAllocateDefaultConsoleProperties();
    if (ConsoleProps == NULL) {
        YoriLibFreeStringContents(&WorkingDir);
        YoriLibFreeStringContents(&FullShortcutPath);
        YoriLibFreeStringContents(&LocalExePath);
        return FALSE;
    }

    memcpy(ConsoleProps->ColorTable, YoriPkgCgaColorTable, sizeof(YoriPkgCgaColorTable));
    ConsoleProps->WindowColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

    YoriLibConstantString(&Description, _T("Yori"));

    if (!YoriLibCreateShortcut(&FullShortcutPath, &LocalExePath, NULL, &Description, &WorkingDir, &LocalExePath, ConsoleProps, 0, 1, (WORD)-1, TRUE, TRUE)) {
        YoriLibDereference(ConsoleProps);
        YoriLibFreeStringContents(&WorkingDir);
        YoriLibFreeStringContents(&LocalExePath);
        YoriLibFreeStringContents(&FullShortcutPath);
        return FALSE;
    }

    YoriLibDereference(ConsoleProps);
    YoriLibFreeStringContents(&WorkingDir);
    YoriLibFreeStringContents(&LocalExePath);
    YoriLibFreeStringContents(&FullShortcutPath);

    return TRUE;
}

/**
 Create a shortcut to a Yori shell on the user's desktop.

 @param YoriExeFullPath Optionally points to a string specifying the shell
        executable.  If not supplied, Yori.exe within the directory containing
        the running process is used.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriPkgCreateDesktopShortcut(
    __in_opt PYORI_STRING YoriExeFullPath
    )
{
    YORI_STRING RelativeShortcutPath;
    YoriLibConstantString(&RelativeShortcutPath, _T("~Desktop\\Yori.lnk"));
    return YoriPkgCreateAppShortcut(&RelativeShortcutPath, YoriExeFullPath);
}

/**
 Create a shortcut to a Yori shell on the user's start menu.

 @param YoriExeFullPath Optionally points to a string specifying the shell
        executable.  If not supplied, Yori.exe within the directory containing
        the running process is used.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriPkgCreateStartMenuShortcut(
    __in_opt PYORI_STRING YoriExeFullPath
    )
{
    YORI_STRING RelativeShortcutPath;
    YoriLibConstantString(&RelativeShortcutPath, _T("~Programs\\Yori.lnk"));
    return YoriPkgCreateAppShortcut(&RelativeShortcutPath, YoriExeFullPath);
}

/**
 Update the login shell to execute the specified program.  If the program
 is not specified, Yori.exe from the directory containing the current
 process is used.

 @param YoriExeFullPath Pointer to the program to use as a login shell.  If
        not specified, Yori.exe from the directory containing the current
        process is used.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriPkgInstallYoriAsLoginShell(
    __in_opt PYORI_STRING YoriExeFullPath
    )
{
    YORI_STRING LocalExePath;
    DWORD Attributes;

    if (YoriExeFullPath == NULL) {
        if (!YoriPkgGetYoriExecutablePath(&LocalExePath)) {
            return FALSE;
        }
    } else {
        YoriLibCloneString(&LocalExePath, YoriExeFullPath);
    }

    Attributes = GetFileAttributes(LocalExePath.StartOfString);
    if (Attributes == (DWORD)-1) {
        YoriLibFreeStringContents(&LocalExePath);
        return FALSE;
    }

    if (!YoriPkgUpdateLogonShell(&LocalExePath)) {
        YoriLibFreeStringContents(&LocalExePath);
        return FALSE;
    }

    YoriLibFreeStringContents(&LocalExePath);
    return TRUE;
}

/**
 Update the login shell to execute the specified program.  If the program
 is not specified, Yui.exe from the directory containing the current
 process is used.

 @param YuiExeFullPath Pointer to the program to use as a login shell.  If
        not specified, Yui.exe from the directory containing the current
        process is used.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriPkgInstallYuiAsLoginShell(
    __in_opt PYORI_STRING YuiExeFullPath
    )
{
    YORI_STRING LocalExePath;
    DWORD Attributes;

    if (YuiExeFullPath == NULL) {
        if (!YoriPkgGetYuiExecutablePath(&LocalExePath)) {
            return FALSE;
        }
    } else {
        YoriLibCloneString(&LocalExePath, YuiExeFullPath);
    }

    Attributes = GetFileAttributes(LocalExePath.StartOfString);
    if (Attributes == (DWORD)-1) {
        YoriLibFreeStringContents(&LocalExePath);
        return FALSE;
    }

    if (!YoriPkgUpdateLogonShell(&LocalExePath)) {
        YoriLibFreeStringContents(&LocalExePath);
        return FALSE;
    }

    YoriLibFreeStringContents(&LocalExePath);
    return TRUE;
}

/**
 Update the login shell to be the default Windows program.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriPkgRestoreLoginShell(
    VOID
    )
{
    return YoriPkgRestoreRegistryLoginShell();
}

/**
 Update the OpenSSH shell to execute the specified program.  If the program
 is not specified, Yori.exe from the directory containing the current
 process is used.

 @param YoriExeFullPath Pointer to the program to use as a login shell.  If
        not specified, Yori.exe from the directory containing the current
        process is used.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriPkgInstallYoriAsOpenSSHShell(
    __in_opt PYORI_STRING YoriExeFullPath
    )
{
    YORI_STRING LocalExePath;
    YORI_STRING KeyName;
    YORI_STRING ValueName;
    DWORD Attributes;

    if (YoriExeFullPath == NULL) {
        if (!YoriPkgGetYoriExecutablePath(&LocalExePath)) {
            return FALSE;
        }
    } else {
        YoriLibCloneString(&LocalExePath, YoriExeFullPath);
    }

    Attributes = GetFileAttributes(LocalExePath.StartOfString);
    if (Attributes == (DWORD)-1) {
        YoriLibFreeStringContents(&LocalExePath);
        return FALSE;
    }

    YoriLibConstantString(&KeyName, _T("SOFTWARE\\OpenSSH"));
    YoriLibConstantString(&ValueName, _T("DefaultShell"));

    if (!YoriPkgUpdateRegistryShell(&KeyName, &ValueName, &LocalExePath)) {
        YoriLibFreeStringContents(&LocalExePath);
        return FALSE;
    }

    YoriLibFreeStringContents(&LocalExePath);
    return TRUE;
}

/**
 Load colors from a scheme file and set them as default in the user's
 registry.

 @param SchemeFile Pointer to the scheme file.

 @param ConsoleTitle Pointer to the console title to apply settings to.
        If not specified, global values are changed which apply to any
        unknown title.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriPkgSetSchemeAsDefault(
    __in PCYORI_STRING SchemeFile,
    __in_opt PCYORI_STRING ConsoleTitle
    )
{
    YORI_CONSOLE_SCREEN_BUFFER_INFOEX BufferInfoEx;
    YORI_STRING FullFileName;
    UCHAR Color;

    YoriLibInitEmptyString(&FullFileName);
    if (!YoriLibUserStringToSingleFilePath(SchemeFile, TRUE, &FullFileName)) {
        return FALSE;
    }

    if (!YoriLibLoadColorTableFromScheme(&FullFileName, BufferInfoEx.ColorTable)) {
        YoriLibFreeStringContents(&FullFileName);
        return FALSE;
    }

    if (!YoriLibLoadWindowColorFromScheme(&FullFileName, &Color)) {
        YoriLibFreeStringContents(&FullFileName);
        return FALSE;
    }

    BufferInfoEx.wAttributes = Color;

    if (!YoriLibLoadPopupColorFromScheme(&FullFileName, &Color)) {
        YoriLibFreeStringContents(&FullFileName);
        return FALSE;
    }

    BufferInfoEx.wPopupAttributes = Color;
    YoriLibFreeStringContents(&FullFileName);

    return YoriPkgSetConsoleDefaults(ConsoleTitle, BufferInfoEx.ColorTable, (UCHAR)BufferInfoEx.wAttributes, (UCHAR)BufferInfoEx.wPopupAttributes);
}




// vim:sw=4:ts=4:et:
