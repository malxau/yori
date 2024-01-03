/**
 * @file move/move.c
 *
 * Yori shell move or rename files
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, MOVEESS OR
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
CHAR strMoveHelpText[] =
        "\n"
        "Moves or renames one or more files.\n"
        "\n"
        "MOVE [-license] [-b] [-k] [-p] <src>\n"
        "MOVE [-license] [-b] [-k] [-p] <src> [<src> ...] <dest>\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -p             Move with POSIX semantics\n"
        "   -k             Keep existing files, do not overwrite\n";

/**
 Display usage text to the user.
 */
BOOL
MoveHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Move %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strMoveHelpText);
    return TRUE;
}


/**
 A context passed between each source file match when moving multiple
 files.
 */
typedef struct _MOVE_CONTEXT {

    /**
     Path to the destination for the move operation.
     */
    YORI_STRING Dest;

    /**
     The file system attributes of the destination.  Used to determine if
     the destination exists and is a directory.
     */
    DWORD DestAttributes;

    /**
     The number of files that have been previously moved.  This can be used
     to determine if we're about to move a second object over the top of
     an earlier moved file.
     */
    DWORD FilesMoved;

    /**
     TRUE if existing files should be replaced, FALSE if they should be kept.
     */
    BOOLEAN ReplaceExisting;

    /**
     TRUE if the move should use POSIX semantics, where in use files are
     removed from the namespace immediately.
     */
    BOOLEAN PosixSemantics;

} MOVE_CONTEXT, *PMOVE_CONTEXT;

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Specifies the recursion depth.  Ignored in this application.

 @param Context Pointer to the move context specifying the destination of the
        move.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
MoveFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    PMOVE_CONTEXT MoveContext = (PMOVE_CONTEXT)Context;
    YORI_STRING FullDest;
    DWORD LastError;
    BOOLEAN TrailingSlash;

    UNREFERENCED_PARAMETER(Depth);

    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    YoriLibInitEmptyString(&FullDest);

    //
    //  If the destination ends in a trailing slash, assume it's a
    //  directory, even if the object is not found.
    //

    TrailingSlash = FALSE;
    if (MoveContext->Dest.LengthInChars > 0 &&
        YoriLibIsSep(MoveContext->Dest.StartOfString[MoveContext->Dest.LengthInChars - 1])) {
        TrailingSlash = TRUE;
    }

    //
    //  For directories, concatenate the desired file name with the directory
    //  target.  For files, the entire path is the intended target.
    //

    if (MoveContext->DestAttributes & FILE_ATTRIBUTE_DIRECTORY || TrailingSlash) {
        YORI_STRING DestWithFile;
        if (!YoriLibAllocateString(&DestWithFile, MoveContext->Dest.LengthInChars + 1 + (YORI_ALLOC_SIZE_T)_tcslen(FileInfo->cFileName) + 1)) {
            return FALSE;
        }

        //
        //  If the path already ends in a trailing slash, don't add another
        //  one.
        //

        if (TrailingSlash) {
            DestWithFile.LengthInChars = YoriLibSPrintf(DestWithFile.StartOfString, _T("%y%s"), &MoveContext->Dest, FileInfo->cFileName);
        } else {
            DestWithFile.LengthInChars = YoriLibSPrintf(DestWithFile.StartOfString, _T("%y\\%s"), &MoveContext->Dest, FileInfo->cFileName);
        }

        if (!YoriLibGetFullPathNameReturnAllocation(&DestWithFile, TRUE, &FullDest, NULL)) {
            return FALSE;
        }
        YoriLibFreeStringContents(&DestWithFile);
    } else {
        if (!YoriLibGetFullPathNameReturnAllocation(&MoveContext->Dest, TRUE, &FullDest, NULL)) {
            return FALSE;
        }
        if (MoveContext->FilesMoved > 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Attempting to move multiple files over a single file (%s)\n"), FullDest.StartOfString);
            YoriLibFreeStringContents(&FullDest);
            return FALSE;
        }
    }

    LastError = YoriLibMoveFile(FilePath, &FullDest, MoveContext->ReplaceExisting, MoveContext->PosixSemantics);
    if (LastError != ERROR_SUCCESS) {
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("MoveFile failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
    } else {
        MoveContext->FilesMoved++;
    }

    YoriLibDereference(FullDest.StartOfString);
    return TRUE;
}

/**
 A callback that is invoked when a directory cannot be successfully enumerated.

 @param FilePath Pointer to the file path that could not be enumerated.

 @param ErrorCode The Win32 error code describing the failure.

 @param Depth Recursion depth, ignored in this application.

 @param Context Ignored.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
MoveFileEnumerateErrorCallback(
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
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("File or directory not found: %y\n"), &UnescapedFilePath);
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
 The main entrypoint for the move builtin command.
 */
#define ENTRYPOINT YoriCmd_YMOVE
#else
/**
 The main entrypoint for the move standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the move cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process indicating success or failure.
 */
DWORD
ENTRYPOINT(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOLEAN ArgumentUnderstood;
    DWORD FilesProcessed;
    DWORD FileCount;
    DWORD LastFileArg = 0;
    WORD MatchFlags;
    YORI_ALLOC_SIZE_T i;
    MOVE_CONTEXT MoveContext;
    BOOLEAN AllocatedDest;
    BOOLEAN BasicEnumeration;
    YORI_STRING Arg;

    FileCount = 0;
    AllocatedDest = FALSE;
    BasicEnumeration = FALSE;
    MoveContext.ReplaceExisting = TRUE;
    MoveContext.PosixSemantics = FALSE;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                MoveHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2021"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("k")) == 0) {
                MoveContext.ReplaceExisting = FALSE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("p")) == 0) {
                MoveContext.PosixSemantics = TRUE;
                ArgumentUnderstood = TRUE;
            }
        } else {
            ArgumentUnderstood = TRUE;
            FileCount++;
            LastFileArg = i;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (FileCount == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("move: argument missing\n"));
        return EXIT_FAILURE;
    } else if (FileCount == 1) {
        YoriLibConstantString(&MoveContext.Dest, _T("."));
    } else {
        if (!YoriLibUserStringToSingleFilePath(&ArgV[LastFileArg], TRUE, &MoveContext.Dest)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("copy: could not resolve %y\n"), &ArgV[LastFileArg]);
            return EXIT_FAILURE;
        }
        AllocatedDest = TRUE;
        FileCount--;
    }

#if YORI_BUILTIN
    YoriLibCancelEnable(FALSE);
#endif

    MoveContext.DestAttributes = GetFileAttributes(MoveContext.Dest.StartOfString);
    if (MoveContext.DestAttributes == 0xFFFFFFFF) {
        MoveContext.DestAttributes = 0;
    }
    MoveContext.FilesMoved = 0;
    FilesProcessed = 0;

    YoriLibLoadAdvApi32Functions();

    for (i = 1; i < ArgC; i++) {
        if (!YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {
            MatchFlags = YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_RETURN_DIRECTORIES;
            if (BasicEnumeration) {
                MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
            }
            YoriLibForEachFile(&ArgV[i],
                               MatchFlags,
                               0,
                               MoveFileFoundCallback,
                               MoveFileEnumerateErrorCallback,
                               &MoveContext);
            FilesProcessed++;
            if (FilesProcessed == FileCount) {
                break;
            }
        }
    }

    if (AllocatedDest) {
        YoriLibFreeStringContents(&MoveContext.Dest);
    }

    if (MoveContext.FilesMoved == 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
