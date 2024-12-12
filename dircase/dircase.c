/**
 * @file dircase/dircase.c
 *
 * Yori shell mark directories for case sensitive or insensitive semantics
 *
 * Copyright (c) 2021 Malcolm J. Smith
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

/**
 Help text to display to the user.
 */
const
CHAR strDirCaseHelpText[] =
        "\n"
        "Mark directories for case sensitive or insensitive semantics.\n"
        "\n"
        "DIRCASE [-license] [-b] <-ci|-cs> [-s] [<directory>...]\n"
        "\n"
        "   -b             Use basic search criteria for directories only\n"
        "   -ci            Set the directories to case insensitive behavior\n"
        "   -cs            Set the directories to case sensitive behavior\n"
        "   -s             Process directories from all subdirectories\n"
        "   -v             Verbose output\n";

/**
 Display usage text to the user.
 */
BOOL
DirCaseHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("DirCase %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strDirCaseHelpText);
    return TRUE;
}

/**
 Context passed for each file found.
 */
typedef struct _DIRCASE_CONTEXT {

    /**
     TRUE if directories should be case sensitive.  FALSE if they should be
     case insensitive.
     */
    BOOLEAN CaseSensitive;

    /**
     TRUE if enumeration is recursive, FALSE if it is within one directory
     only.
     */
    BOOLEAN Recursive;

    /**
     TRUE if output should be generated for each file processed.  FALSE for
     silent processing.
     */
    BOOLEAN Verbose;

    /**
     Records the total number of directories found.
     */
    LONGLONG DirsFound;

    /**
     Records the total number of directories processed.
     */
    LONGLONG DirsModified;

} DIRCASE_CONTEXT, *PDIRCASE_CONTEXT;

/**
 A callback that is invoked when a directory is found that matches a search
 criteria specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the directory.

 @param Depth Specifies recursion depth.  Ignored in this application.

 @param Context Pointer to the dircase context structure indicating the action
        to perform and populated with the directory found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
DirCaseFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    PDIRCASE_CONTEXT DirCaseContext = (PDIRCASE_CONTEXT)Context;
    YORI_FILE_CASE_SENSITIVE_INFORMATION CaseSensitiveInfo;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE hDir;
    DWORD AccessRequired;
    DWORD Status;

    UNREFERENCED_PARAMETER(FileInfo);
    UNREFERENCED_PARAMETER(Depth);

    ASSERT(YoriLibIsStringNullTerminated(FilePath));
    ASSERT((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);

    DirCaseContext->DirsFound++;

    if (DirCaseContext->Verbose) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Updating %y...\n"), FilePath);
    }

    AccessRequired = FILE_WRITE_ATTRIBUTES | SYNCHRONIZE;

    hDir = CreateFile(FilePath->StartOfString,
                      AccessRequired,
                      FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                      NULL,
                      OPEN_EXISTING,
                      FILE_FLAG_BACKUP_SEMANTICS,
                      NULL);

    if (hDir == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    CaseSensitiveInfo.Flags = 0;
    if (DirCaseContext->CaseSensitive) {
        CaseSensitiveInfo.Flags = 1;
    }


    Status = DllNtDll.pNtSetInformationFile(hDir, &IoStatusBlock, &CaseSensitiveInfo, sizeof(CaseSensitiveInfo), FileCaseSensitiveInformation);
    CloseHandle(hDir);

    if (Status != 0) {
        LPTSTR ErrText = YoriLibGetNtErrorText(Status);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Update of %y failed: %s"), FilePath, ErrText);
        YoriLibFreeWinErrorText(ErrText);
    } else {
        DirCaseContext->DirsModified++;
    }

    return TRUE;
}

/**
 A callback that is invoked when a directory cannot be successfully enumerated.

 @param FilePath Pointer to the file path that could not be enumerated.

 @param ErrorCode The Win32 error code describing the failure.

 @param Depth Recursion depth, ignored in this application.

 @param Context Pointer to the context block indicating whether the
        enumeration was recursive.  Recursive enumerates do not complain
        if a matching file is not in every single directory, because
        common usage expects files to be in a subset of directories only.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
DirCaseFileEnumerateErrorCallback(
    __in PYORI_STRING FilePath,
    __in DWORD ErrorCode,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    YORI_STRING UnescapedFilePath;
    BOOL Result = FALSE;
    PDIRCASE_CONTEXT DirCaseContext = (PDIRCASE_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Depth);

    YoriLibInitEmptyString(&UnescapedFilePath);
    if (!YoriLibUnescapePath(FilePath, &UnescapedFilePath)) {
        UnescapedFilePath.StartOfString = FilePath->StartOfString;
        UnescapedFilePath.LengthInChars = FilePath->LengthInChars;
    }

    if (ErrorCode == ERROR_FILE_NOT_FOUND || ErrorCode == ERROR_PATH_NOT_FOUND) {
        if (!DirCaseContext->Recursive) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("File or directory not found: %y\n"), &UnescapedFilePath);
        }
        Result = TRUE;
    } else {
        LPTSTR ErrText = YoriLibGetWinErrorText(ErrorCode);
        YORI_STRING DirName;
        LPTSTR FilePart;
        YoriLibInitEmptyString(&DirName);
        DirName.StartOfString = UnescapedFilePath.StartOfString;
        FilePart = YoriLibFindRightMostCharacter(&UnescapedFilePath, '\\');
        if (FilePart != NULL) {
            DirName.LengthInChars = (YORI_ALLOC_SIZE_T)(FilePart - DirName.StartOfString);
        } else {
            DirName.LengthInChars = UnescapedFilePath.LengthInChars;
        }
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Enumerate of %y failed: %s"), &DirName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
    }
    YoriLibFreeStringContents(&UnescapedFilePath);
    return Result;
}


#ifdef YORI_BUILTIN
/**
 The main entrypoint for the dircase builtin command.
 */
#define ENTRYPOINT YoriCmd_YDIRCASE
#else
/**
 The main entrypoint for the dircase standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the dircase cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process, zero on success, nonzero on failure.
 */
DWORD
ENTRYPOINT(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOLEAN ArgumentUnderstood;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 0;
    WORD MatchFlags;
    BOOLEAN BasicEnumeration = FALSE;
    BOOLEAN OperationFound;
    DIRCASE_CONTEXT DirCaseContext;
    YORI_STRING Arg;
    DWORD MajorVersion;
    DWORD MinorVersion;
    DWORD BuildNumber;

    ZeroMemory(&DirCaseContext, sizeof(DirCaseContext));
    OperationFound = FALSE;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                DirCaseHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2021"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("ci")) == 0) {
                DirCaseContext.CaseSensitive = FALSE;
                OperationFound = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("cs")) == 0) {
                DirCaseContext.CaseSensitive = TRUE;
                OperationFound = TRUE;
                ArgumentUnderstood = TRUE;

            } else if (YoriLibCompareStringLitIns(&Arg, _T("s")) == 0) {
                DirCaseContext.Recursive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("v")) == 0) {
                DirCaseContext.Verbose = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("-")) == 0) {
                ArgumentUnderstood = TRUE;
                StartArg = i + 1;
                break;
            }
        } else {
            ArgumentUnderstood = TRUE;
            StartArg = i;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (StartArg == 0 || StartArg == ArgC) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("dircase: missing argument\n"));
        return EXIT_FAILURE;
    }

    YoriLibGetOsVersion(&MajorVersion, &MinorVersion, &BuildNumber);

    if (DllNtDll.pNtSetInformationFile == NULL ||
        BuildNumber < 17763) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("dircase: OS support not present\n"));
        return EXIT_FAILURE;
    }

    if (!OperationFound) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("dircase: operation not specified\n"));
        return EXIT_FAILURE;
    }

#if YORI_BUILTIN
    YoriLibCancelEnable(FALSE);
#endif

    MatchFlags = YORILIB_FILEENUM_RETURN_DIRECTORIES;
    if (DirCaseContext.Recursive) {
        MatchFlags |= YORILIB_FILEENUM_RECURSE_BEFORE_RETURN;
    }
    if (BasicEnumeration) {
        MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
    }

    for (i = StartArg; i < ArgC; i++) {

        YoriLibForEachFile(&ArgV[i],
                           MatchFlags,
                           0,
                           DirCaseFileFoundCallback,
                           DirCaseFileEnumerateErrorCallback,
                           &DirCaseContext);
    }

    if (DirCaseContext.DirsFound == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("dircase: no matching files found\n"));
    }

    if (DirCaseContext.DirsModified == 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
