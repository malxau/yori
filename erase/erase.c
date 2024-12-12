/**
 * @file erase/erase.c
 *
 * Yori shell erase files
 *
 * Copyright (c) 2017-2021 Malcolm J. Smith
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
 * FITNESS ERASE A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE ERASE ANY CLAIM, DAMAGES OR OTHER
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
CHAR strEraseHelpText[] =
        "\n"
        "Delete one or more files.\n"
        "\n"
        "ERASE [-license] [-b] [-p | -r] [-s] <file> [<file>...]\n"
        "\n"
        "   --             Treat all further arguments as files to delete\n"
        "   -b             Use basic search criteria for files only\n"
        "   -p             Delete files with POSIX semantics\n"
        "   -r             Send files to the recycle bin\n"
        "   -s             Erase all files matching the pattern in all subdirectories\n";

/**
 Display usage text to the user.
 */
BOOL
EraseHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Erase %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strEraseHelpText);
    return TRUE;
}

/**
 A structure passed to each file found.
 */
typedef struct _ERASE_CONTEXT {

    /**
     TRUE if files should be deleted with POSIX semantics.
     */
    BOOLEAN PosixSemantics;

    /**
     TRUE if files should be sent to the recycle bin.
     */
    BOOLEAN RecycleBin;

    /**
     The number of files found.
     */
    DWORDLONG FilesFound;

    /**
     The number of files successfully marked for delete.
     */
    DWORDLONG FilesMarkedForDelete;

} ERASE_CONTEXT, *PERASE_CONTEXT;

/**
 Delete a file via DeleteFile or via the POSIX delete API depending on which
 command line arguments were specified.

 @param EraseContext Pointer to the context indicating which delete mode to
        use.

 @param FileName Pointer to the file name to delete.

 @return TRUE to indicate the file was marked for delete successfully, or
         FALSE to indicate failure.
 */
BOOL
EraseDeleteFile(
    __in PERASE_CONTEXT EraseContext,
    __in PYORI_STRING FileName
    )
{
    if (!EraseContext->PosixSemantics) {
        return DeleteFile(FileName->StartOfString);
    }

    return YoriLibPosixDeleteFile(FileName);
}

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Recursion depth, ignored in this application.

 @param Context Specifies if erase should move objects to the recycle bin and
        records the count of files found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
EraseFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    DWORD Err;
    LPTSTR ErrText;
    BOOLEAN FileDeleted;
    PERASE_CONTEXT EraseContext = (PERASE_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Depth);

    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    FileDeleted = FALSE;
    if ((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {

        EraseContext->FilesFound++;

        //
        //  If the user wanted it deleted via the recycle bin, try that.
        //

        if (EraseContext->RecycleBin) {
            if (YoriLibRecycleBinFile(FilePath)) {
                FileDeleted = TRUE;
            }
        }

        //
        //  If the user didn't ask for recycle bin or if that failed, delete
        //  directly.
        //

        if (!FileDeleted && !EraseDeleteFile(EraseContext, FilePath)) {
            Err = GetLastError();
            if (Err == ERROR_ACCESS_DENIED) {
                DWORD OldAttributes = GetFileAttributes(FilePath->StartOfString);
                DWORD NewAttributes = OldAttributes & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);

                if (OldAttributes != NewAttributes) {
                    SetFileAttributes(FilePath->StartOfString, NewAttributes);

                    Err = NO_ERROR;

                    if (!EraseDeleteFile(EraseContext, FilePath)) {
                        Err = GetLastError();
                    } else {
                        FileDeleted = TRUE;
                    }

                    if (Err != NO_ERROR) {
                        SetFileAttributes(FilePath->StartOfString, OldAttributes);
                    }
                }
            }

            if (Err != NO_ERROR) {
                ErrText = YoriLibGetWinErrorText(Err);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("erase: delete of %y failed: %s"), FilePath, ErrText);
                YoriLibFreeWinErrorText(ErrText);
            }
        } else {
            FileDeleted = TRUE;
        }

        if (FileDeleted) {
            EraseContext->FilesMarkedForDelete++;
        }
    }
    return TRUE;
}

/**
 A callback that is invoked when a directory cannot be successfully enumerated.

 @param FilePath Pointer to the file path that could not be enumerated.

 @param ErrorCode The Win32 error code describing the failure.

 @param Depth Recursion depth, ignored in this application.

 @param Context Context, ignored in this function.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
EraseFileEnumerateErrorCallback(
    __in PYORI_STRING FilePath,
    __in DWORD ErrorCode,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    YORI_STRING UnescapedFilePath;
    BOOL Result = FALSE;

    UNREFERENCED_PARAMETER(Depth);
    UNREFERENCED_PARAMETER(Context);

    YoriLibInitEmptyString(&UnescapedFilePath);
    if (!YoriLibUnescapePath(FilePath, &UnescapedFilePath)) {
        UnescapedFilePath.StartOfString = FilePath->StartOfString;
        UnescapedFilePath.LengthInChars = FilePath->LengthInChars;
    }

    if (ErrorCode == ERROR_FILE_NOT_FOUND || ErrorCode == ERROR_PATH_NOT_FOUND) {
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
 The main entrypoint for the erase builtin command.
 */
#define ENTRYPOINT YoriCmd_YERASE
#else
/**
 The main entrypoint for the erase standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the erase cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ENTRYPOINT(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOLEAN ArgumentUnderstood;
    WORD MatchFlags;
    BOOLEAN Recursive;
    BOOLEAN BasicEnumeration;
    YORI_ALLOC_SIZE_T StartArg = 0;
    YORI_ALLOC_SIZE_T i;
    ERASE_CONTEXT Context;
    YORI_STRING Arg;

    ZeroMemory(&Context, sizeof(Context));
    Recursive = FALSE;
    BasicEnumeration = FALSE;

    for (i = 1; i < ArgC; i++) {

        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));
        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                EraseHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2021"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("p")) == 0) {
                Context.PosixSemantics = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("r")) == 0) {
                Context.RecycleBin = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("s")) == 0) {
                Recursive = TRUE;
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

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("erase: missing argument\n"));
        return EXIT_FAILURE;
    }

    if (Context.PosixSemantics &&
        DllKernel32.pSetFileInformationByHandle == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("erase: OS support not present\n"));
        return EXIT_FAILURE;
    }

#if YORI_BUILTIN
    YoriLibCancelEnable(FALSE);
#endif

    //
    //  Attempt to enable backup privilege so an administrator can access more
    //  objects successfully.
    //

    YoriLibEnableBackupPrivilege();

    MatchFlags = YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_DIRECTORY_CONTENTS;
    if (Recursive) {
        MatchFlags |= YORILIB_FILEENUM_RECURSE_BEFORE_RETURN | YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;
    }
    if (BasicEnumeration) {
        MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
    }

    for (i = StartArg; i < ArgC; i++) {

        YoriLibForEachStream(&ArgV[i],
                             MatchFlags,
                             0,
                             EraseFileFoundCallback,
                             EraseFileEnumerateErrorCallback,
                             &Context);
    }

    if (Context.FilesFound == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("erase: no matching files found\n"));
        ASSERT(Context.FilesMarkedForDelete == 0);
    }

    if (Context.FilesMarkedForDelete == 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
