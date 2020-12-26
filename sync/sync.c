/**
 * @file sync/sync.c
 *
 * Yori create files or update timestamps
 *
 * Copyright (c) 2018 Malcolm J. Smith
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
CHAR strSyncHelpText[] =
        "\n"
        "Flush files, directories or volumes to disk.\n"
        "\n"
        "SYNC [-license] [-b] [-s] [-v] <file>...\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -q             Query if the volume is in use, and flush if it is not in use\n"
        "   -r             Dismount and remount the volume\n"
        "   -s             Process files from all subdirectories\n"
        "   -v             Display verbose output\n";

/**
 Display usage text to the user.
 */
BOOL
SyncHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Sync %i.%02i\n"), SYNC_VER_MAJOR, SYNC_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strSyncHelpText);
    return TRUE;
}

/**
 Context passed to the callback which is invoked for each file found.
 */
typedef struct _SYNC_CONTEXT {

    /**
     Counts the number of files processed in an enumerate.  If this is zero,
     the program assumes the request is to create a new file.
     */
    DWORD FilesFoundThisArg;

    /**
     TRUE if the volume should be locked.  In Windows, this will fail if the
     volume is in use, otherwise it will flush the volume ready for removal.
     */
    BOOL LockVolume;

    /**
     TRUE if the volume should be dismounted.  In Windows, this will probably
     be remounted again immediately, hence externally calling this a remount
     command.
     */
    BOOL VolumeDismount;

    /**
     If TRUE, display output for each object where sync is attempted.
     */
    BOOL Verbose;

} SYNC_CONTEXT, *PSYNC_CONTEXT;

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.  Note in this application this can
        be NULL when it is operating on files that do not yet exist.

 @param Depth Specifies the recursion depth.  Ignored in this application.

 @param Context Pointer to the sync context structure indicating the
        action to perform and populated with the file and line count found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
SyncFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in_opt PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    HANDLE FileHandle;
    DWORD DesiredAccess;
    DWORD LastError;
    DWORD BytesReturned;
    LPTSTR ErrText;
    PSYNC_CONTEXT SyncContext = (PSYNC_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Depth);
    UNREFERENCED_PARAMETER(FileInfo);

    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    SyncContext->FilesFoundThisArg++;

    DesiredAccess = GENERIC_WRITE;

    //
    //  If the user requested a volume operation, find the volume name and
    //  attempt the operation on the volume.  If not, just use the file name
    //  that has already been located.
    //

    if (SyncContext->VolumeDismount || SyncContext->LockVolume) {
        YORI_STRING VolumePath;
        YoriLibInitEmptyString(&VolumePath);
        if (!YoriLibGetVolumePathName(FilePath, &VolumePath)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("sync: could not determine volume for %y\n"), FilePath);
            return TRUE;
        }

        if (SyncContext->Verbose) {
            if (SyncContext->VolumeDismount) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("sync: dismounting %y\n"), &VolumePath);
            } else {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("sync: locking %y\n"), &VolumePath);
            }
        }

        FileHandle = CreateFile(VolumePath.StartOfString,
                                DesiredAccess,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                                NULL);

        if (FileHandle == NULL || FileHandle == INVALID_HANDLE_VALUE) {
            LastError = GetLastError();
            ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("sync: open of %y failed: %s"), &VolumePath, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            YoriLibFreeStringContents(&VolumePath);
            return TRUE;
        }

        if (SyncContext->VolumeDismount) {
            if (!DeviceIoControl(FileHandle, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &BytesReturned, NULL)) {
                LastError = GetLastError();
                ErrText = YoriLibGetWinErrorText(LastError);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("sync: dismount of %y failed: %s"), &VolumePath, ErrText);
                YoriLibFreeWinErrorText(ErrText);
                YoriLibFreeStringContents(&VolumePath);
                return TRUE;
            }
        } else {
            if (!DeviceIoControl(FileHandle, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &BytesReturned, NULL)) {
                LastError = GetLastError();
                ErrText = YoriLibGetWinErrorText(LastError);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("sync: lock of %y failed, volume may be in use: %s"), &VolumePath, ErrText);
                YoriLibFreeWinErrorText(ErrText);
                YoriLibFreeStringContents(&VolumePath);
                return TRUE;
            }
        }

        YoriLibFreeStringContents(&VolumePath);
        CloseHandle(FileHandle);
        return TRUE;

    } else {

        if (SyncContext->Verbose) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("sync: syncing %y\n"), FilePath);
        }

        FileHandle = CreateFile(FilePath->StartOfString,
                                DesiredAccess,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                NULL,
                                OPEN_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                                NULL);

        if (FileHandle == NULL || FileHandle == INVALID_HANDLE_VALUE) {
            LastError = GetLastError();
            ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("sync: open of %y failed: %s"), FilePath, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            return TRUE;
        }

        if (!FlushFileBuffers(FileHandle)) {
            LastError = GetLastError();
            ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("sync: flush of %y failed: %s"), FilePath, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            return TRUE;
        }

        CloseHandle(FileHandle);
        return TRUE;
    }
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the sync builtin command.
 */
#define ENTRYPOINT YoriCmd_SYNC
#else
/**
 The main entrypoint for the sync standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the sync cmdlet.

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
    BOOL Recursive = FALSE;
    BOOL BasicEnumeration = FALSE;
    SYNC_CONTEXT SyncContext;
    YORI_STRING Arg;

    ZeroMemory(&SyncContext, sizeof(SyncContext));

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                SyncHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("q")) == 0) {
                SyncContext.LockVolume = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("r")) == 0) {
                SyncContext.VolumeDismount = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                Recursive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("v")) == 0) {
                SyncContext.Verbose = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("-")) == 0) {
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

    //
    //  Attempt to enable backup privilege so an administrator can access more
    //  objects successfully.
    //

    YoriLibEnableBackupPrivilege();

    //
    //  If no file name is specified, we have nothing to flush.
    //

    if (StartArg == 0 || StartArg == ArgC) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("sync: missing argument\n"));
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

            SyncContext.FilesFoundThisArg = 0;
            YoriLibForEachStream(&ArgV[i], MatchFlags, 0, SyncFileFoundCallback, NULL, &SyncContext);
            if (SyncContext.FilesFoundThisArg == 0) {
                YORI_STRING FullPath;
                YoriLibInitEmptyString(&FullPath);
                if (YoriLibUserStringToSingleFilePath(&ArgV[i], TRUE, &FullPath)) {
                    SyncFileFoundCallback(&FullPath, NULL, 0, &SyncContext);
                    YoriLibFreeStringContents(&FullPath);
                }
            }
        }
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
