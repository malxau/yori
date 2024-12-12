/**
 * @file touch/touch.c
 *
 * Yori create files or update timestamps
 *
 * Copyright (c) 2018-2022 Malcolm J. Smith
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
CHAR strTouchHelpText[] =
        "\n"
        "Create files or update timestamps.\n"
        "\n"
        "TOUCH [-license] [-a] [-b] [-c] [-e] [-f size] [-h] [-s] [-t <date and time>]\n"
        "      [-w] <file>...\n"
        "\n"
        "   -a             Update last access time\n"
        "   -b             Use basic search criteria for files only\n"
        "   -c             Update create time\n"
        "   -e             Only update existing files\n"
        "   -f             Create new file with specified file size\n"
        "   -h             Operate on links as opposed to link targets\n"
        "   -s             Process files from all subdirectories\n"
        "   -t             Specify the timestamp to set\n"
        "   -w             Update write time\n";

/**
 Display usage text to the user.
 */
BOOL
TouchHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Touch %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strTouchHelpText);
    return TRUE;
}

/**
 Context passed to the callback which is invoked for each file found.
 */
typedef struct _TOUCH_CONTEXT {

    /**
     Specifies the new creation time to apply to each file.
     */
    FILETIME NewCreationTime;

    /**
     Specifies the new access time to apply to each file.
     */
    FILETIME NewAccessTime;

    /**
     Specifies the new write time to apply to each file.
     */
    FILETIME NewWriteTime;

    /**
     File size for newly created files.
     */
    LARGE_INTEGER NewFileSize;

    /**
     Counts the number of files processed in an enumerate.  If this is zero,
     the program assumes the request is to create a new file.
     */
    ULONG FilesFoundThisArg;

    /**
     If TRUE, only existing files should be modified, and no new files should
     be created.
     */
    BOOLEAN ExistingOnly;

    /**
     If TRUE, changes should be applied to links as opposed to link targets.
     */
    BOOLEAN NoFollowLinks;

} TOUCH_CONTEXT, *PTOUCH_CONTEXT;

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.  Note in this application this can
        be NULL when it is operating on files that do not yet exist.

 @param Depth Specifies the recursion depth.  Ignored in this application.

 @param Context Pointer to the touch context structure indicating the
        action to perform and populated with the file and line count found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
TouchFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in_opt PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    HANDLE FileHandle;
    DWORD DesiredAccess;
    DWORD OpenFlags;
    PTOUCH_CONTEXT TouchContext = (PTOUCH_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Depth);
    UNREFERENCED_PARAMETER(FileInfo);

    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    DesiredAccess = GENERIC_READ | FILE_WRITE_ATTRIBUTES;
    if (FileInfo == NULL) {
        DesiredAccess |= GENERIC_WRITE;
    }

    OpenFlags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS;
    if (TouchContext->NoFollowLinks) {
        OpenFlags = OpenFlags | FILE_FLAG_OPEN_REPARSE_POINT;
    }

    FileHandle = CreateFile(FilePath->StartOfString,
                            DesiredAccess,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL,
                            TouchContext->ExistingOnly?OPEN_EXISTING:OPEN_ALWAYS,
                            OpenFlags,
                            NULL);

    if (FileHandle == NULL || FileHandle == INVALID_HANDLE_VALUE) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("touch: open of %y failed: %s"), FilePath, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return TRUE;
    }

    TouchContext->FilesFoundThisArg++;

    if (FileInfo == NULL) {
        if (TouchContext->NewFileSize.QuadPart != 0) {
            SetFilePointer(FileHandle, TouchContext->NewFileSize.LowPart, &TouchContext->NewFileSize.HighPart, FILE_BEGIN);
            if (!SetEndOfFile(FileHandle)) {
                DWORD LastError = GetLastError();
                LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("touch: setting file size of %y failed: %s"), FilePath, ErrText);
                YoriLibFreeWinErrorText(ErrText);

                //
                //  Intentional fallout
                //
            }
        }
    }

    if (!SetFileTime(FileHandle, &TouchContext->NewCreationTime, &TouchContext->NewAccessTime, &TouchContext->NewWriteTime)) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("touch: updating timestamps of %y failed: %s"), FilePath, ErrText);
        YoriLibFreeWinErrorText(ErrText);

        //
        //  Intentional fallout
        //
    }

    CloseHandle(FileHandle);
    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the touch builtin command.
 */
#define ENTRYPOINT YoriCmd_TOUCH
#else
/**
 The main entrypoint for the touch standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the touch cmdlet.

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
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 0;
    WORD MatchFlags;
    BOOLEAN Recursive = FALSE;
    BOOLEAN BasicEnumeration = FALSE;
    BOOLEAN UpdateLastAccess = FALSE;
    BOOLEAN UpdateCreationTime = FALSE;
    BOOLEAN UpdateWriteTime = FALSE;
    SYSTEMTIME CurrentSystemTime;
    FILETIME TimestampToUse;
    TOUCH_CONTEXT TouchContext;
    YORI_STRING Arg;

    ZeroMemory(&TouchContext, sizeof(TouchContext));
    GetSystemTime(&CurrentSystemTime);
    if (!SystemTimeToFileTime(&CurrentSystemTime, &TimestampToUse)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("touch: could not query system time\n"));
        return EXIT_FAILURE;
    }

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                TouchHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018-2022"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("a")) == 0) {
                UpdateLastAccess = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("c")) == 0) {
                UpdateCreationTime = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("e")) == 0) {
                TouchContext.ExistingOnly = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("f")) == 0) {
                if (i + 1 < ArgC) {
                    TouchContext.NewFileSize = YoriLibStringToFileSize(&ArgV[i + 1]);
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("h")) == 0) {
                TouchContext.NoFollowLinks = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("s")) == 0) {
                Recursive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("t")) == 0) {
                if (i + 1 < ArgC) {
                    SYSTEMTIME NewTime;
                    FILETIME LocalNewTime;
                    ZeroMemory(&NewTime, sizeof(NewTime));
                    if (YoriLibStringToDateTime(&ArgV[i + 1], &NewTime)) {
                        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT,
                                      _T("Setting time to %i/%i/%i:%i:%i:%i\n"),
                                      NewTime.wYear,
                                      NewTime.wMonth,
                                      NewTime.wDay,
                                      NewTime.wHour,
                                      NewTime.wMinute,
                                      NewTime.wSecond);
                        if (!SystemTimeToFileTime(&NewTime, &LocalNewTime) || !LocalFileTimeToFileTime(&LocalNewTime, &TimestampToUse)) {
                            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Could not parse time: %y\n"), &ArgV[i + 1]);
                        }
                        ArgumentUnderstood = TRUE;
                    }
                    i++;
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("w")) == 0) {
                UpdateWriteTime = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("-")) == 0) {
                StartArg = i + 1;
                ArgumentUnderstood = TRUE;
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

    if (!UpdateLastAccess && !UpdateCreationTime && !UpdateWriteTime) {
        UpdateWriteTime = TRUE;
    }

    if (UpdateLastAccess) {
        TouchContext.NewAccessTime = TimestampToUse;
    } else {
        TouchContext.NewAccessTime.dwLowDateTime = (DWORD)-1;
        TouchContext.NewAccessTime.dwHighDateTime = (DWORD)-1;
    }

    if (UpdateCreationTime) {
        TouchContext.NewCreationTime = TimestampToUse;
    } else {
        TouchContext.NewCreationTime.dwLowDateTime = (DWORD)-1;
        TouchContext.NewCreationTime.dwHighDateTime = (DWORD)-1;
    }

    if (UpdateWriteTime) {
        TouchContext.NewWriteTime = TimestampToUse;
    } else {
        TouchContext.NewWriteTime.dwLowDateTime = (DWORD)-1;
        TouchContext.NewWriteTime.dwHighDateTime = (DWORD)-1;
    }

    //
    //  If no file name is specified, use stdin; otherwise open
    //  the file and use that
    //

    if (StartArg == 0 || StartArg == ArgC) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("touch: missing argument\n"));
        return EXIT_FAILURE;
    } else {
        MatchFlags = YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_RETURN_DIRECTORIES;
        if (Recursive) {
            MatchFlags |= YORILIB_FILEENUM_RECURSE_BEFORE_RETURN | YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;
        }
        if (BasicEnumeration) {
            MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
        }

        for (i = StartArg; i < ArgC; i++) {

            TouchContext.FilesFoundThisArg = 0;
            YoriLibForEachFile(&ArgV[i], MatchFlags, 0, TouchFileFoundCallback, NULL, &TouchContext);
            if (TouchContext.FilesFoundThisArg == 0) {
                YORI_STRING FullPath;
                YoriLibInitEmptyString(&FullPath);
                if (YoriLibUserStringToSingleFilePath(&ArgV[i], TRUE, &FullPath)) {
                    TouchFileFoundCallback(&FullPath, NULL, 0, &TouchContext);
                    YoriLibFreeStringContents(&FullPath);
                }
            }
        }
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
