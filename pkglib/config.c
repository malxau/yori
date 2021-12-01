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
    DWORD Index;
    YORI_STRING ParentDirectory;
    DWORD BytesNeeded;
    DWORD BytesWritten;
    YORI_STRING LocalExePath;
    YORI_STRING EscapedExePath;
    YORI_STRING ProfileFileName;
    LPSTR MultibyteEscapedExePath;

    if (!YoriPkgGetTerminalProfilePath(&ProfileFileName)) {
        return FALSE;
    }

    if (YoriExeFullPath == NULL) {
        YORI_STRING RelativePath;
        YoriLibConstantString(&RelativePath, _T("~APPDIR\\Yori.exe"));
        YoriLibInitEmptyString(&LocalExePath);
        if (!YoriLibUserStringToSingleFilePath(&RelativePath, TRUE, &LocalExePath)) {
            YoriLibFreeStringContents(&ProfileFileName);
            return FALSE;
        }
    } else {
        memcpy(&LocalExePath, YoriExeFullPath, sizeof(YORI_STRING));
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

// vim:sw=4:ts=4:et:
