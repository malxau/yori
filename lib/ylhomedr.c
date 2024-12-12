/**
 * @file lib/ylhomedr.c
 *
 * Expansion of home directory locations to their full counterparts.
 *
 * Copyright (c) 2017-2018 Malcolm J. Smith
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

#include "yoripch.h"
#include "yorilib.h"

/**
 Convert a specified shell folder, by a known folder GUID, into its string
 form.  This function is only available in Vista+.

 @param FolderType The identifier of the directory.

 @param ExpandedSymbol On successful completion, populated with a string
        identifying the path to the directory.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibExpandShellDirectoryGuid(
    __in CONST GUID * FolderType,
    __inout PYORI_STRING ExpandedSymbol
    )
{
    PWSTR ExpandedString;
    YORI_ALLOC_SIZE_T LocationLength;

    YoriLibLoadShell32Functions();
    YoriLibLoadOle32Functions();

    if (DllShell32.pSHGetKnownFolderPath == NULL ||
        DllOle32.pCoTaskMemFree == NULL) {
        return FALSE;
    }


    if (DllShell32.pSHGetKnownFolderPath(FolderType, 0, NULL, &ExpandedString) != 0) {
        return FALSE;
    }

    LocationLength = (YORI_ALLOC_SIZE_T)_tcslen(ExpandedString);

    if (!YoriLibAllocateString(ExpandedSymbol, LocationLength + 1)) {
        DllOle32.pCoTaskMemFree(ExpandedString);
        return FALSE;
    }

    memcpy(ExpandedSymbol->StartOfString, ExpandedString, (LocationLength + 1) * sizeof(TCHAR));
    ExpandedSymbol->LengthInChars = LocationLength;

    DllOle32.pCoTaskMemFree(ExpandedString);
    return TRUE;
}


/**
 Convert a specified shell folder, by identifier, into its string form.

 @param FolderType The identifier of the directory.

 @param ExpandedSymbol On successful completion, populated with a string
        identifying the path to the directory.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibExpandShellDirectory(
    __in INT FolderType,
    __inout PYORI_STRING ExpandedSymbol
    )
{
    YoriLibLoadShell32Functions();
    if (DllShell32.pSHGetSpecialFolderPathW == NULL) {
        YoriLibLoadShfolderFunctions();
        if (DllShfolder.pSHGetFolderPathW == NULL) {
            return FALSE;
        }
    }

    if (!YoriLibAllocateString(ExpandedSymbol, MAX_PATH)) {
        return FALSE;
    }

    ExpandedSymbol->StartOfString[0] = '\0';

    if (DllShell32.pSHGetSpecialFolderPathW != NULL) {
        if (DllShell32.pSHGetSpecialFolderPathW(NULL, ExpandedSymbol->StartOfString, FolderType, FALSE) < 0) {
            YoriLibFreeStringContents(ExpandedSymbol);
            return FALSE;
        }
    } else {
        if (!SUCCEEDED(DllShfolder.pSHGetFolderPathW(NULL, FolderType, NULL, 0, ExpandedSymbol->StartOfString))) {
            YoriLibFreeStringContents(ExpandedSymbol);
            return FALSE;
        }
    }

    ExpandedSymbol->LengthInChars = (YORI_ALLOC_SIZE_T)_tcslen(ExpandedSymbol->StartOfString);

    return TRUE;
}

/**
 A mapping between a '~' prefixed special directory name and a CSIDL that the
 shell uses to identify it.
 */
typedef struct _YORI_LIB_CSIDL_MAP {

    /**
     The special directory name.
     */
    LPTSTR DirName;

    /**
     The corresponding CSIDL.
     */
    DWORD Csidl;
} YORI_LIB_CSIDL_MAP, *PYORI_LIB_CSIDL_MAP;

/**
 A table of special directory names whose locations can be obtained via
 SHGetSpecialFolderPath or SHGetFolderPath.
 */
CONST YORI_LIB_CSIDL_MAP YoriLibSpecialDirectoryMap[] = {
    {_T("~APPDATA"),            CSIDL_APPDATA},
    {_T("~APPDATALOCAL"),       CSIDL_LOCALAPPDATA},
    {_T("~COMMONAPPDATA"),      CSIDL_COMMON_APPDATA},
    {_T("~COMMONDESKTOP"),      CSIDL_COMMON_DESKTOPDIRECTORY},
    {_T("~COMMONDOCUMENTS"),    CSIDL_COMMON_DOCUMENTS},
    {_T("~COMMONPROGRAMS"),     CSIDL_COMMON_PROGRAMS},
    {_T("~COMMONSTART"),        CSIDL_COMMON_STARTMENU},
    {_T("~DESKTOP"),            CSIDL_DESKTOPDIRECTORY},
    {_T("~DOCUMENTS"),          CSIDL_PERSONAL},
    {_T("~LOCALAPPDATA"),       CSIDL_LOCALAPPDATA},
    {_T("~PROGRAMFILES"),       CSIDL_PROGRAM_FILES},
#ifdef _WIN64
    {_T("~PROGRAMFILES32"),     CSIDL_PROGRAM_FILESX86},
    {_T("~PROGRAMFILES64"),     CSIDL_PROGRAM_FILES},
#else
    {_T("~PROGRAMFILES32"),     CSIDL_PROGRAM_FILES},
#endif
    {_T("~PROGRAMS"),           CSIDL_PROGRAMS},
    {_T("~START"),              CSIDL_STARTMENU},
    {_T("~STARTUP"),            CSIDL_STARTUP},
    {_T("~SYSTEM"),             CSIDL_SYSTEM},
    {_T("~WINDOWS"),            CSIDL_WINDOWS}
};

/**
 Translate a special directory name into its expanded form if the directory
 name is defined via a CSIDL that can be resolved with SHGetSpecialFolderPath
 et al.

 @param SymbolToExpand The special directory name, prefixed with '~'.

 @param ExpandedSymbol On successful completion, populated with the directory
        corresponding to the special directory name.

 @return TRUE to indicate a match was found and expansion successfully
         performed, FALSE if the symbol name should be checked for other
         matches.
 */
__success(return)
BOOL
YoriLibExpandDirectoryFromMap(
    __in PYORI_STRING SymbolToExpand,
    __inout PYORI_STRING ExpandedSymbol
    )
{
    DWORD Index;

    for (Index = 0; Index < sizeof(YoriLibSpecialDirectoryMap)/sizeof(YoriLibSpecialDirectoryMap[0]); Index++) {
        if (YoriLibCompareStringLitIns(SymbolToExpand, YoriLibSpecialDirectoryMap[Index].DirName) == 0) {
            return YoriLibExpandShellDirectory(YoriLibSpecialDirectoryMap[Index].Csidl, ExpandedSymbol);
        }
    }

    return FALSE;
}

/**
 Expand a directory component in a path, specified via a tilde and description,
 into its corresponding physical directory.

 @param SymbolToExpand The tilde based directory reference.

 @param ExpandedSymbol On successful completion, populated with the directory
        corresponding to the reference.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibExpandHomeSymbol(
    __in PYORI_STRING SymbolToExpand,
    __inout PYORI_STRING ExpandedSymbol
    )
{
    if (YoriLibCompareStringLitIns(SymbolToExpand, _T("~")) == 0) {
        DWORD BytesNeeded;
        BytesNeeded = GetEnvironmentVariable(_T("HOMEDRIVE"), NULL, 0) + GetEnvironmentVariable(_T("HOMEPATH"), NULL, 0);
        if (!YoriLibIsSizeAllocatable(BytesNeeded)) {
            return FALSE;
        }

        if (!YoriLibAllocateString(ExpandedSymbol, (YORI_ALLOC_SIZE_T)BytesNeeded)) {
            return FALSE;
        }

        ExpandedSymbol->LengthInChars = (YORI_ALLOC_SIZE_T)GetEnvironmentVariable(_T("HOMEDRIVE"), ExpandedSymbol->StartOfString, ExpandedSymbol->LengthAllocated);
        ExpandedSymbol->LengthInChars = ExpandedSymbol->LengthInChars +
                                        (YORI_ALLOC_SIZE_T)GetEnvironmentVariable(_T("HOMEPATH"),
                                            &ExpandedSymbol->StartOfString[ExpandedSymbol->LengthInChars],
                                            ExpandedSymbol->LengthAllocated - ExpandedSymbol->LengthInChars);
        return TRUE;
    } else if (YoriLibCompareStringLitIns(SymbolToExpand, _T("~APPDIR")) == 0) {
        LPTSTR FinalSlash;

        //
        //  Unlike most other Win32 APIs, this one has no way to indicate
        //  how much space it needs.  We can be wasteful here though, since
        //  it'll be freed immediately.
        //

        if (!YoriLibAllocateString(ExpandedSymbol, 32768)) {
            return FALSE;
        }

        ExpandedSymbol->LengthInChars = (YORI_ALLOC_SIZE_T)GetModuleFileName(NULL, ExpandedSymbol->StartOfString, ExpandedSymbol->LengthAllocated);
        FinalSlash = YoriLibFindRightMostCharacter(ExpandedSymbol, '\\');
        if (FinalSlash == NULL) {
            YoriLibFreeStringContents(ExpandedSymbol);
            return FALSE;
        }

        ExpandedSymbol->LengthInChars = (YORI_ALLOC_SIZE_T)(FinalSlash - ExpandedSymbol->StartOfString);
        return TRUE;
    } else if (YoriLibExpandDirectoryFromMap(SymbolToExpand, ExpandedSymbol)) {
        return TRUE;
    } else if (YoriLibCompareStringLitIns(SymbolToExpand, _T("~DOWNLOADS")) == 0) {
        BOOL Result;

        //
        //  If a Vista era function to find the Downloads folder exists,
        //  use it.
        //

        YoriLibLoadShell32Functions();
        if (DllShell32.pSHGetKnownFolderPath != NULL) {
            return YoriLibExpandShellDirectoryGuid(&FOLDERID_Downloads, ExpandedSymbol);
        }

        //
        //  If not, Downloads is a subdirectory of Documents.  This function
        //  allocates a MAX_PATH buffer because the underlying API doesn't
        //  specify a length.
        //

        Result = YoriLibExpandShellDirectory(CSIDL_PERSONAL, ExpandedSymbol);
        if (Result) {
            if (ExpandedSymbol->LengthInChars + sizeof("\\Downloads") < ExpandedSymbol->LengthAllocated) {
                memcpy(&ExpandedSymbol->StartOfString[ExpandedSymbol->LengthInChars],
                       _T("\\Downloads"),
                       (sizeof("\\Downloads") - 1) * sizeof(TCHAR));
                ExpandedSymbol->LengthInChars = ExpandedSymbol->LengthInChars + sizeof("\\Downloads") - 1;
                return TRUE;
            }
        }
    }

    ExpandedSymbol->StartOfString = SymbolToExpand->StartOfString;
    ExpandedSymbol->LengthInChars = SymbolToExpand->LengthInChars;
    return TRUE;
}

/**
 Expand all tilde based home references in a file path and return the
 expanded form.

 @param FileString A string specifying a path that may include tilde based
        directory references.

 @param ExpandedString On successful completion, contains the corresponding
        full path.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibExpandHomeDirectories(
    __in PYORI_STRING FileString,
    __out PYORI_STRING ExpandedString
    )
{
    YORI_ALLOC_SIZE_T CharIndex;
    YORI_STRING BeforeSymbol;
    YORI_STRING AfterSymbol;
    YORI_STRING SymbolToExpand;
    YORI_STRING ExpandedSymbol;
    BOOL PreviousWasSeperator = TRUE;

    for (CharIndex = 0; CharIndex < FileString->LengthInChars; CharIndex++) {
        if (FileString->StartOfString[CharIndex] == '~' &&
            (CharIndex == 0 || YoriLibIsSep(FileString->StartOfString[CharIndex - 1]))) {

            YoriLibInitEmptyString(&BeforeSymbol);
            YoriLibInitEmptyString(&AfterSymbol);
            YoriLibInitEmptyString(&SymbolToExpand);

            BeforeSymbol.StartOfString = FileString->StartOfString;
            BeforeSymbol.LengthInChars = CharIndex;

            SymbolToExpand.StartOfString = &FileString->StartOfString[CharIndex];
            SymbolToExpand.LengthInChars = 0;

            while (CharIndex < FileString->LengthInChars && !YoriLibIsSep(FileString->StartOfString[CharIndex])) {
                SymbolToExpand.LengthInChars++;
                CharIndex++;
            }

            AfterSymbol.StartOfString = &FileString->StartOfString[CharIndex];
            AfterSymbol.LengthInChars = FileString->LengthInChars - CharIndex;

            YoriLibInitEmptyString(&ExpandedSymbol);

            if (!YoriLibExpandHomeSymbol(&SymbolToExpand, &ExpandedSymbol)) {
                return FALSE;
            }

            YoriLibInitEmptyString(ExpandedString);
            YoriLibYPrintf(ExpandedString, _T("%y%y%y"), &BeforeSymbol, &ExpandedSymbol, &AfterSymbol);
            YoriLibFreeStringContents(&ExpandedSymbol);
            if (ExpandedString->StartOfString != NULL) {
                return TRUE;
            }
            return FALSE;
        }

        if (YoriLibIsSep(FileString->StartOfString[CharIndex])) {
            PreviousWasSeperator = TRUE;
        } else {
            PreviousWasSeperator = FALSE;
        }
    }

    return FALSE;
}

/**
 Return TRUE if the argument is a special DOS device name, FALSE if it is
 a regular file.  DOS device names include things like AUX, CON, PRN etc.
 In Yori, a DOS device name is only a DOS device name if it does not
 contain any path information.

 @param File The file name to check.

 @return TRUE to indicate this argument is a DOS device name, FALSE to
         indicate that it is a regular file.
 */
BOOL
YoriLibIsFileNameDeviceName(
    __in PCYORI_STRING File
    )
{
    YORI_STRING NameToCheck;
    YORI_ALLOC_SIZE_T Offset;
    BOOLEAN Prefixed;

    YoriLibInitEmptyString(&NameToCheck);
    Offset = 0;
    Prefixed = FALSE;
    if (YoriLibIsPathPrefixed(File)) {
        Offset = sizeof("\\\\.\\") - 1;
        Prefixed = TRUE;
    }

    NameToCheck.StartOfString = &File->StartOfString[Offset];
    NameToCheck.LengthInChars = File->LengthInChars - Offset;


    //
    //  If it's \\.\x: treat it as a device.  Note this cannot have any
    //  trailing characters, or it'd be a file.
    //

    if (Prefixed && 
        NameToCheck.LengthInChars == 2) {

        if (YoriLibIsDriveLetterWithColon(&NameToCheck)) {
            return TRUE;
        }
    }

    //
    //  Check for a physical drive name.  Note this also checks that the
    //  prefix is a dot.
    //

    if (Prefixed) {
        if (YoriLibCompareStringLitInsCnt(File, _T("\\\\.\\PHYSICALDRIVE"), sizeof("\\\\.\\PHYSICALDRIVE") - 1) == 0 ||
            YoriLibCompareStringLitInsCnt(File, _T("\\\\.\\HARDDISK"), sizeof("\\\\.\\HARDDISK") - 1) == 0 ||
            YoriLibCompareStringLitInsCnt(File, _T("\\\\.\\CDROM"), sizeof("\\\\.\\CDROM") - 1) == 0) {
            return TRUE;
        }
    }

    if (NameToCheck.LengthInChars < 3 || NameToCheck.LengthInChars > 4) {
        return FALSE;
    }

    if (YoriLibCompareStringLitIns(&NameToCheck, _T("CON")) == 0 ||
        YoriLibCompareStringLitIns(&NameToCheck, _T("AUX")) == 0 ||
        YoriLibCompareStringLitIns(&NameToCheck, _T("PRN")) == 0 ||
        YoriLibCompareStringLitIns(&NameToCheck, _T("NUL")) == 0) {

        return TRUE;
    }

    if (NameToCheck.LengthInChars < 4) {
        return FALSE;
    }

    if (YoriLibCompareStringLitInsCnt(&NameToCheck, _T("LPT"), 3) == 0 &&
        (NameToCheck.StartOfString[3] >= '1' && NameToCheck.StartOfString[3] <= '9')) {

        return TRUE;
    }

    if (YoriLibCompareStringLitInsCnt(&NameToCheck, _T("COM"), 3) == 0 &&
        (NameToCheck.StartOfString[3] >= '1' && NameToCheck.StartOfString[3] <= '9')) {

        return TRUE;
    }


    return FALSE;
}

/**
 Resolves a user string which must refer to a single file into a physical path
 for that file.

 @param UserString The string as specified by the user.  This is currently
        required to be NULL terminated.

 @param ReturnEscapedPath TRUE if the resulting path should be prefixed with
        \\?\, FALSE to indicate a traditional Win32 path.

 @param FullPath On successful completion, this pointer is updated to point to
        the full path string.  This string should be uninitialized on input and
        is allocated within this routine.  The caller is expected to free this
        with @ref YoriLibFreeStringContents.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibUserStringToSingleFilePath(
    __in PCYORI_STRING UserString,
    __in BOOL ReturnEscapedPath,
    __out PYORI_STRING FullPath
    )
{
    YORI_STRING PathToTranslate;
    YORI_STRING YsFilePrefix;
    YORI_STRING ExpandedString;
    BOOL ReturnValue = FALSE;

    YoriLibInitEmptyString(&YsFilePrefix);
    YsFilePrefix.StartOfString = UserString->StartOfString;
    YsFilePrefix.LengthInChars = sizeof("file:///") - 1;

    YoriLibInitEmptyString(&PathToTranslate);
    if (YoriLibCompareStringLitIns(&YsFilePrefix, _T("file:///")) == 0) {
        PathToTranslate.StartOfString = &UserString->StartOfString[YsFilePrefix.LengthInChars];
        PathToTranslate.LengthInChars = UserString->LengthInChars - YsFilePrefix.LengthInChars;
    } else {
        PathToTranslate.StartOfString = UserString->StartOfString;
        PathToTranslate.LengthInChars = UserString->LengthInChars;
    }

    YoriLibInitEmptyString(FullPath);

    if (YoriLibExpandHomeDirectories(&PathToTranslate, &ExpandedString)) {
        ReturnValue = YoriLibGetFullPathNameReturnAllocation(&ExpandedString, ReturnEscapedPath, FullPath, NULL);
        YoriLibFreeStringContents(&ExpandedString);
    } else {
        ReturnValue = YoriLibGetFullPathNameReturnAllocation(&PathToTranslate, ReturnEscapedPath, FullPath, NULL);
    }

    if (ReturnValue == 0) {
        YoriLibFreeStringContents(FullPath);
        return 0;
    }

    return ReturnValue;
}

/**
 Checks if a file name refers to a device, and if so returns a path to the
 device.  If it does not refer to a device, the path is resolved into a file
 path for the specified file.

 @param UserString The string as specified by the user.  This is currently
        required to be NULL terminated.

 @param ReturnEscapedPath TRUE if the resulting path should be prefixed with
        \\?\, FALSE to indicate a traditional Win32 path.

 @param FullPath On successful completion, this pointer is updated to point to
        the full path string.  This string should be uninitialized on input and
        is allocated within this routine.  The caller is expected to free this
        with @ref YoriLibFreeStringContents.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibUserStringToSingleFilePathOrDevice(
    __in PCYORI_STRING UserString,
    __in BOOL ReturnEscapedPath,
    __out PYORI_STRING FullPath
    )
{
    if (YoriLibIsFileNameDeviceName(UserString)) {
        YORI_ALLOC_SIZE_T CharsNeeded;
        BOOL Prefixed;

        CharsNeeded = UserString->LengthInChars + 1;
        Prefixed = YoriLibIsPathPrefixed(UserString);

        if (!Prefixed && ReturnEscapedPath) {
            CharsNeeded += sizeof("\\\\.\\") - 1;
        }
        if (!YoriLibAllocateString(FullPath, CharsNeeded)) {
            return FALSE;
        }
        if (!Prefixed && ReturnEscapedPath) {
            FullPath->LengthInChars = YoriLibSPrintf(FullPath->StartOfString, _T("\\\\.\\%y"), UserString);
        } else {
            FullPath->LengthInChars = YoriLibSPrintf(FullPath->StartOfString, _T("%y"), UserString);
        }
        return TRUE;
    }
    return YoriLibUserStringToSingleFilePath(UserString, ReturnEscapedPath, FullPath);
}

// vim:sw=4:ts=4:et:
