/**
 * @file du/du.c
 *
 * Yori shell display space used by files in directories
 *
 * Copyright (c) 2019 Malcolm J. Smith
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
CHAR strDuHelpText[] =
        "\n"
        "Display disk space used within directories.\n"
        "\n"
        "DU [-license] [-b] [-color] [-r <num>] [-s <size>] [<spec>...]\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -color         Use file color highlighting\n"
        "   -r <num>       The maximum recursion depth to display\n"
        "   -s <size>      Only display directories containing at least size bytes\n";

/**
 Display usage text to the user.
 */
BOOL
DuHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Du %i.%i\n"), DU_VER_MAJOR, DU_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strDuHelpText);
    return TRUE;
}

/**
 The maximum size of the stream name in WIN32_FIND_STREAM_DATA.
 */
#define DU_MAX_STREAM_NAME          (MAX_PATH + 36)

/**
 A private definition of WIN32_FIND_STREAM_DATA in case the compilation
 environment doesn't provide it.
 */
typedef struct _DU_WIN32_FIND_STREAM_DATA {

    /**
     The length of the stream, in bytes.
     */
    LARGE_INTEGER StreamSize;

    /**
     The name of the stream.
     */
    WCHAR cStreamName[DU_MAX_STREAM_NAME];
} DU_WIN32_FIND_STREAM_DATA, *PDU_WIN32_FIND_STREAM_DATA;

/**
 A structure describing a particular directory.  When traversing through
 files to calculate space, there will be one of these structures for each
 parent component of the file.
 */
typedef struct _DU_DIRECTORY_STACK {

    /**
     The name of this directory, in escaped form.
     */
    YORI_STRING DirectoryName;

    /**
     The number of files or directories encountered within this directory.
     */
    LONGLONG ObjectsFoundThisDirectory;

    /**
     The amount of bytes consumed by files within this directory.
     */
    LONGLONG SpaceConsumedThisDirectory;

    /**
     The amount of bytes consumed by subdirectories within this directory.
     Note this is populated only when the subdirectories have completed
     their enumerations.
     */
    LONGLONG SpaceConsumedInChildren;
} DU_DIRECTORY_STACK, *PDU_DIRECTORY_STACK;

/**
 Context passed to the callback which is invoked for each file found.
 */
typedef struct _DU_CONTEXT {

    /**
     An array of directory components corresponding to the components within
     the path currently being parsed.  File sizes are added to the leafmost
     element (ie., StackIndex) and when a change to a parent component is
     detected, size from the child component is propagated to the parent and
     the child component is prepared for reuse by the next child directory.
     */
    PDU_DIRECTORY_STACK DirStack;

    /**
     Index of the last element in the directory stack that has been operated
     on.
     */
    DWORD StackIndex;

    /**
     The number of allocated elements in the DirStack array.  Will be zero
     if the array has not been allocated yet.
     */
    DWORD StackAllocated;

    /**
     The maximum depth to display.  This is a user specified value allowing
     recursion to terminate at a particular level to display a summary view
     only at a particular depth.
     */
    DWORD MaximumDepthToDisplay;

    /**
     The minimum directory size to display.
     */
    LARGE_INTEGER MinimumDirectorySizeToDisplay;

    /**
     Color information to display against matching files.
     */
    YORI_LIB_FILE_FILTER ColorRules;

} DU_CONTEXT, *PDU_CONTEXT;

/**
 Deallocate all child allocations within a DU_CONTEXT structure.  The
 structure itself is typically stack allocated and will not be freed.

 @param DuContext Pointer to the DuContext to clean up.
 */
VOID
DuCleanupContext(
    __in PDU_CONTEXT DuContext
    )
{
    DWORD Index;

    for (Index = 0; Index < DuContext->StackAllocated; Index++) {
        YoriLibFreeStringContents(&DuContext->DirStack[Index].DirectoryName);
    }

    if (DuContext->DirStack != NULL) {
        YoriLibFree(DuContext->DirStack);
        DuContext->DirStack = NULL;
    }

    DuContext->StackAllocated = 0;
    DuContext->StackIndex = 0;
    YoriLibFileFiltFreeFilter(&DuContext->ColorRules);
}

/**
 Clear the contents of a directory frame so it can be reused.

 @param DirStack Pointer to the directory frame to clear.
 */
VOID
DuCloseStack(
    __in PDU_DIRECTORY_STACK DirStack
    )
{
    //
    //  Note the DirectoryName string remains allocated in the hope that the
    //  next directory can use it.
    //

    DirStack->DirectoryName.LengthInChars = 0;
    DirStack->ObjectsFoundThisDirectory = 0;
    DirStack->SpaceConsumedThisDirectory = 0;
    DirStack->SpaceConsumedInChildren = 0;
}

/**
 Print the space consumed by a particular directory, and close out the
 directory's stack frame so it can be reused by the next directory.

 @param DuContext Pointer to the DuContext which contains the directory to
        display and close.

 @param Depth Specifies the array index of the directory to display and close.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
DuReportAndCloseStack(
    __in PDU_CONTEXT DuContext,
    __in DWORD Depth
    )
{
    YORI_STRING UnescapedPath;
    PYORI_STRING StringToDisplay;
    YORI_STRING FileSizeString;
    TCHAR FileSizeStringBuffer[8];
    LARGE_INTEGER SizeToDisplay;

    PDU_DIRECTORY_STACK DirStack;

    DirStack = &DuContext->DirStack[Depth];

    if (DuContext->MaximumDepthToDisplay == 0 ||
        Depth <= DuContext->MaximumDepthToDisplay) {

        SizeToDisplay.QuadPart = DirStack->SpaceConsumedInChildren + DirStack->SpaceConsumedThisDirectory;

        if (DuContext->MinimumDirectorySizeToDisplay.QuadPart == 0 ||
            SizeToDisplay.QuadPart >= DuContext->MinimumDirectorySizeToDisplay.QuadPart) {

            YoriLibInitEmptyString(&UnescapedPath);
            if (YoriLibUnescapePath(&DirStack->DirectoryName, &UnescapedPath)) {
                StringToDisplay = &UnescapedPath;
            } else {
                StringToDisplay = &DirStack->DirectoryName;
            }

            YoriLibInitEmptyString(&FileSizeString);
            FileSizeString.StartOfString = FileSizeStringBuffer;
            FileSizeString.LengthAllocated = sizeof(FileSizeStringBuffer)/sizeof(FileSizeStringBuffer[0]);
            YoriLibFileSizeToString(&FileSizeString, &SizeToDisplay);
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y %y\n"), &FileSizeString, StringToDisplay);

            YoriLibFreeStringContents(&UnescapedPath);
        }
    }

    DuCloseStack(DirStack);
    return TRUE;
}

/**
 Display all directory frames which have not been otherwise displayed because
 no object has been found in a subsequent directory.

 @param DuContext Pointer to the DuContext which may contain active directory
        frames.

 @param MinDepthToDisplay Indicates the minimum depth number that should be
        displayed to the user.  Frames below this are cleared for reuse but
        not displayed.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
DuReportAndCloseAllActiveStacks(
    __in PDU_CONTEXT DuContext,
    __in DWORD MinDepthToDisplay
    )
{
    DWORD Index;
    Index = DuContext->StackIndex;
    if (Index >= DuContext->StackAllocated) {
        ASSERT(Index == 0);
        ASSERT(DuContext->DirStack == NULL);
        return TRUE;
    }
    while (TRUE) {
        if (Index > 0) {
            DuContext->DirStack[Index - 1].SpaceConsumedInChildren += DuContext->DirStack[Index].SpaceConsumedInChildren + DuContext->DirStack[Index].SpaceConsumedThisDirectory;
        }
        if (Index >= MinDepthToDisplay) {
            DuReportAndCloseStack(DuContext, Index);
        } else {
            DuCloseStack(&DuContext->DirStack[Index]);
        }
        if (Index == 0) {
            break;
        }
        Index--;
        DuContext->StackIndex--;
    }
    return TRUE;
}


/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Recursion depth, ignored in this application.

 @param Context Pointer to the du context structure indicating the
        action to perform and populated with the number of objects found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
DuFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    PDU_CONTEXT DuContext = (PDU_CONTEXT)Context;
    LPTSTR FilePart;
    DWORD Index;

    if (Depth >= DuContext->StackAllocated) {
        PDU_DIRECTORY_STACK NewStack;
        NewStack = YoriLibMalloc((Depth + 1) * sizeof(DU_DIRECTORY_STACK));
        if (NewStack == NULL) {
            return FALSE;
        }

        if (DuContext->StackAllocated > 0) {
            memcpy(NewStack, DuContext->DirStack, DuContext->StackAllocated * sizeof(DU_DIRECTORY_STACK));
            YoriLibFree(DuContext->DirStack);
        }

        for (Index = DuContext->StackAllocated; Index <= Depth; Index++) {
            YoriLibInitEmptyString(&NewStack[Index].DirectoryName);
            NewStack[Index].ObjectsFoundThisDirectory = 0;
            NewStack[Index].SpaceConsumedThisDirectory = 0;
            NewStack[Index].SpaceConsumedInChildren = 0;
        }

        DuContext->DirStack = NewStack;
        DuContext->StackAllocated = Depth + 1;
    }

    //
    //  StackIndex would normally be populated except for the first item at
    //  Depth == 0
    //

    if (DuContext->StackIndex > 0 || DuContext->DirStack[0].DirectoryName.LengthInChars > 0) {
        Index = DuContext->StackIndex;
        while (TRUE) {
            DWORD StackDirLength;
            StackDirLength = DuContext->DirStack[Index].DirectoryName.LengthInChars;
            if (Depth >= Index &&
                YoriLibCompareStringCount(FilePath, &DuContext->DirStack[Index].DirectoryName, StackDirLength) == 0 &&
                (FilePath->LengthInChars == StackDirLength ||
                 YoriLibIsSep(FilePath->StartOfString[StackDirLength]))) {

                break;
            }

            if (Index > 0) {
                DuContext->DirStack[Index - 1].SpaceConsumedInChildren += DuContext->DirStack[Index].SpaceConsumedInChildren + DuContext->DirStack[Index].SpaceConsumedThisDirectory;
            }
            DuReportAndCloseStack(DuContext, Index);
            if (Index == 0) {
                break;
            }
            Index--;
            DuContext->StackIndex--;
        }
    }

    FilePart = YoriLibFindRightMostCharacter(FilePath, '\\');
    ASSERT(FilePart != NULL);
    if (FilePart != NULL) {
        YORI_STRING ThisDirName;
        YoriLibInitEmptyString(&ThisDirName);
        ThisDirName.StartOfString = FilePath->StartOfString;
        ThisDirName.LengthInChars = (DWORD)(FilePart - FilePath->StartOfString);

        Index = Depth;
        while (TRUE) {
            if (DuContext->DirStack[Index].DirectoryName.LengthInChars > 0) {
                ASSERT(Index == DuContext->StackIndex);
                ASSERT(YoriLibCompareString(&DuContext->DirStack[Index].DirectoryName, &ThisDirName) == 0);
                break;
            }
            if (DuContext->DirStack[Index].DirectoryName.LengthAllocated <= ThisDirName.LengthInChars) {
                YoriLibFreeStringContents(&DuContext->DirStack[Index].DirectoryName);
                if (!YoriLibAllocateString(&DuContext->DirStack[Index].DirectoryName, ThisDirName.LengthInChars + 1)) {
                    return FALSE;
                }
            }
            memcpy(DuContext->DirStack[Index].DirectoryName.StartOfString, ThisDirName.StartOfString, ThisDirName.LengthInChars * sizeof(TCHAR));
            DuContext->DirStack[Index].DirectoryName.StartOfString[ThisDirName.LengthInChars] = '\0';
            DuContext->DirStack[Index].DirectoryName.LengthInChars = ThisDirName.LengthInChars;
            ASSERT(DuContext->DirStack[Index].ObjectsFoundThisDirectory == 0);
            ASSERT(DuContext->DirStack[Index].SpaceConsumedThisDirectory == 0);
            ASSERT(DuContext->DirStack[Index].SpaceConsumedInChildren == 0);
            if (Index == 0) {
                break;
            }
            Index--;
            FilePart = YoriLibFindRightMostCharacter(&ThisDirName, '\\');
            ASSERT(FilePart != NULL);
            ThisDirName.LengthInChars = (DWORD)(FilePart - ThisDirName.StartOfString);
        }
    }

    DuContext->StackIndex = Depth;
    DuContext->DirStack[Depth].ObjectsFoundThisDirectory++;

    if ((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        LARGE_INTEGER FileSize;
        FileSize.LowPart = FileInfo->nFileSizeLow;
        FileSize.HighPart = FileInfo->nFileSizeHigh;
        //YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("   adding %lli bytes for %y to %y Depth %i\n"), FileSize.QuadPart, FilePath, &DuContext->DirStack[Depth].DirectoryName, Depth);
        DuContext->DirStack[Depth].SpaceConsumedThisDirectory += FileSize.QuadPart;
    }

    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the du builtin command.
 */
#define ENTRYPOINT YoriCmd_YDU
#else
/**
 The main entrypoint for the du standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the du cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 0;
    DWORD MatchFlags;
    BOOL BasicEnumeration = FALSE;
    BOOL DisplayColor = FALSE;
    DU_CONTEXT DuContext;
    YORI_STRING Arg;

    ZeroMemory(&DuContext, sizeof(DuContext));

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                DuHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("color")) == 0) {
                DisplayColor = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("r")) == 0) {
                if (i + 1 < ArgC) {
                    LONGLONG Depth;
                    DWORD CharsConsumed;
                    YoriLibStringToNumber(&ArgV[i + 1], TRUE, &Depth, &CharsConsumed);
                    if (CharsConsumed > 0) {
                        DuContext.MaximumDepthToDisplay = (DWORD)Depth;
                    }
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                if (i + 1 < ArgC) {
                    DuContext.MinimumDirectorySizeToDisplay = YoriLibStringToFileSize(&ArgV[i + 1]);
                    ArgumentUnderstood = TRUE;
                    i++;
                }
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

    if (DisplayColor) {
        YORI_STRING ErrorSubstring;
        YORI_STRING Combined;
        if (YoriLibLoadCombinedFileColorString(NULL, &Combined)) {
            if (!YoriLibFileFiltParseColorString(&DuContext.ColorRules, &Combined, &ErrorSubstring)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("du: parse error at %y\n"), &ErrorSubstring);
            }
            YoriLibFreeStringContents(&Combined);
        }
    }

    MatchFlags = YORILIB_FILEENUM_RETURN_FILES |
                 YORILIB_FILEENUM_RETURN_DIRECTORIES |
                 YORILIB_FILEENUM_RECURSE_BEFORE_RETURN;
    if (BasicEnumeration) {
        MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
    }

    //
    //  If no file name is specified, use .
    //

    if (StartArg == 0) {
        YORI_STRING FilesInDirectorySpec;
        YoriLibConstantString(&FilesInDirectorySpec, _T("."));
        YoriLibForEachFile(&FilesInDirectorySpec, MatchFlags, 0, DuFileFoundCallback, &DuContext);
        DuReportAndCloseAllActiveStacks(&DuContext, 1);
    } else {
        for (i = StartArg; i < ArgC; i++) {
            YoriLibForEachFile(&ArgV[i], MatchFlags, 0, DuFileFoundCallback, &DuContext);
            DuReportAndCloseAllActiveStacks(&DuContext, 1);
        }
    }

    DuCleanupContext(&DuContext);

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et: