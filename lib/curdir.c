/**
 * @file lib/curdir.c
 *
 * Support for storing and retrieving current directory information.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "yoripch.h"
#include "yorilib.h"

/**
 A global variable of the directory string to display for current directory.
 This value is the result of comparing each component in the path to the on
 disk form to ensure that the case matches objects found on disk.
 */
YORI_STRING YoriLibCurrentDirectoryForDisplay;

/**
 Interrogate each component in the specified path against the file system and
 return a string with the case of each component modified to match objects
 found from disk.  Any component not found or obtainable will be retained as
 is.

 @param Path Pointer to the path to convert.

 @param OnDiskCasePath On successful completion, updated to point to a newly
        allocated string where path components may be updated to refer to
        on disk case.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibGetOnDiskCaseForPath(
    __in PYORI_STRING Path,
    __out PYORI_STRING OnDiskCasePath
    )
{
    DWORD Index;
    DWORD LastSepOffset;
    HANDLE FindHandle;
    WIN32_FIND_DATA FindData;
    YORI_STRING SearchComponent;
    YORI_STRING NewPath;
    YORI_STRING EffectiveRoot;
    TCHAR LastSepChar;

    if (Path->LengthInChars == 0) {
        YoriLibInitEmptyString(OnDiskCasePath);
        return TRUE;
    }

    if (!YoriLibFindEffectiveRoot(Path, &EffectiveRoot)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&NewPath, Path->LengthInChars + 1)) {
        return FALSE;
    }

    memcpy(NewPath.StartOfString, Path->StartOfString, Path->LengthInChars * sizeof(TCHAR));
    NewPath.LengthInChars = Path->LengthInChars;
    NewPath.StartOfString[NewPath.LengthInChars] = '\0';

    while (NewPath.LengthInChars > EffectiveRoot.LengthInChars &&
           YoriLibIsSep(NewPath.StartOfString[NewPath.LengthInChars - 1])) {

        NewPath.LengthInChars--;
        NewPath.StartOfString[NewPath.LengthInChars] = '\0';
    }

    LastSepOffset = NewPath.LengthInChars;

    for (Index = NewPath.LengthInChars; Index >= EffectiveRoot.LengthInChars; Index--) {
        if (YoriLibIsSep(NewPath.StartOfString[Index - 1])) {
            SearchComponent.StartOfString = &NewPath.StartOfString[Index];
            SearchComponent.LengthInChars = LastSepOffset - Index;
            if (SearchComponent.LengthInChars > 0) {
                LastSepChar = '\0';
                if (LastSepOffset < NewPath.LengthInChars) {
                    LastSepChar = NewPath.StartOfString[LastSepOffset];
                    NewPath.StartOfString[LastSepOffset] = '\0';
                }
                FindHandle = FindFirstFile(NewPath.StartOfString, &FindData);
                if (FindHandle != INVALID_HANDLE_VALUE) {
                    if (YoriLibCompareStringWithLiteralInsensitive(&SearchComponent, FindData.cFileName) == 0) { 
                        memcpy(SearchComponent.StartOfString, FindData.cFileName, SearchComponent.LengthInChars * sizeof(TCHAR));
                    } else if (YoriLibCompareStringWithLiteralInsensitive(&SearchComponent, FindData.cAlternateFileName) == 0) { 
                        memcpy(SearchComponent.StartOfString, FindData.cAlternateFileName, SearchComponent.LengthInChars * sizeof(TCHAR));
                    }
                    FindClose(FindHandle);
                }
                NewPath.StartOfString[LastSepOffset] = LastSepChar;
            }
            LastSepOffset = Index - 1;
        }
    }

    if (YoriLibIsDriveLetterWithColonAndSlash(&NewPath)) {
        NewPath.StartOfString[0] = YoriLibUpcaseChar(NewPath.StartOfString[0]);
    } else if (YoriLibIsPrefixedDriveLetterWithColonAndSlash(&NewPath)) {
        NewPath.StartOfString[4] = YoriLibUpcaseChar(NewPath.StartOfString[4]);
    }

    memcpy(OnDiskCasePath, &NewPath, sizeof(YORI_STRING));
    return TRUE;
}

/**
 Return the current directory on a drive without changing the current
 directory.

 @param Drive The drive letter whose current directory should be returned.

 @param DriveCurrentDirectory On successful completion, populated with the
        current directory for the specified drive.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibGetCurrentDirectoryOnDrive(
    __in TCHAR Drive,
    __out PYORI_STRING DriveCurrentDirectory
    )
{
    TCHAR EnvVariableName[4];
    DWORD DriveCurrentDirectoryLength;

    EnvVariableName[0] = '=';
    EnvVariableName[1] = Drive;
    EnvVariableName[2] = ':';
    EnvVariableName[3] = '\0';

    DriveCurrentDirectoryLength = GetEnvironmentVariable(EnvVariableName, NULL, 0);

    if (DriveCurrentDirectoryLength > 0) {
        if (!YoriLibAllocateString(DriveCurrentDirectory, DriveCurrentDirectoryLength)) {
            return FALSE;
        }

        DriveCurrentDirectory->LengthInChars = GetEnvironmentVariable(EnvVariableName, DriveCurrentDirectory->StartOfString, DriveCurrentDirectory->LengthAllocated);
        if (DriveCurrentDirectory->LengthInChars == 0 || DriveCurrentDirectory->LengthInChars >= DriveCurrentDirectoryLength) {
            YoriLibFreeStringContents(DriveCurrentDirectory);
            return FALSE;
        }
    } else {
        if (!YoriLibAllocateString(DriveCurrentDirectory, 4)) {
            return FALSE;
        }
        YoriLibSPrintf(DriveCurrentDirectory->StartOfString, _T("%c:\\"), Drive);
        DriveCurrentDirectory->LengthInChars = 3;
    }
    return TRUE;
}

/**
 Update the current directory on a drive without changing the current
 directory.

 @param Drive The drive letter whose current directory should be updated.

 @param DriveCurrentDirectory The directory to set as the current directory
        on the specified drive.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibSetCurrentDirectoryOnDrive(
    __in TCHAR Drive,
    __in PYORI_STRING DriveCurrentDirectory
    )
{
    TCHAR EnvVariableName[4];
    LPTSTR szDriveCurrentDirectory;
    BOOL Result;

    EnvVariableName[0] = '=';
    EnvVariableName[1] = Drive;
    EnvVariableName[2] = ':';
    EnvVariableName[3] = '\0';

    szDriveCurrentDirectory = YoriLibMalloc((DriveCurrentDirectory->LengthInChars + 1) * sizeof(TCHAR));
    if (szDriveCurrentDirectory == NULL) {
        return FALSE;
    }

    YoriLibSPrintf(szDriveCurrentDirectory, _T("%y"), DriveCurrentDirectory);

    Result = SetEnvironmentVariable(EnvVariableName, szDriveCurrentDirectory);
    YoriLibFree(szDriveCurrentDirectory);
    return Result;
}

/**
 Allocate a new string and query the current directory from the operating
 system into that allocation.

 @param CurrentDirectory Pointer to a string which will be allocated as part
        of this routine and updated to refer to the process current directory.
        This string will be NULL terminated within this routine.  The caller
        should free this with YoriLibFreeStringContents.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibGetCurrentDirectory(
    __out PYORI_STRING CurrentDirectory
    )
{
    YORI_STRING String;
    DWORD CharsNeeded;

    CharsNeeded = GetCurrentDirectory(0, NULL);

    while(TRUE) {

        if (!YoriLibAllocateString(&String, CharsNeeded + 1)) {
            return FALSE;
        }
    
    
        String.LengthInChars = GetCurrentDirectory(String.LengthAllocated, String.StartOfString);
    
        if (String.LengthInChars == 0) {
            YoriLibFreeStringContents(&String);
            return FALSE;
        }

        if (String.LengthInChars < String.LengthAllocated) {
            String.StartOfString[String.LengthInChars] = '\0';
            memcpy(CurrentDirectory, &String, sizeof(YORI_STRING));
            return TRUE;
        }

        CharsNeeded = String.LengthInChars;

        YoriLibFreeStringContents(&String);
    }
}

/**
 Return a reference to a string containing the current directory where
 components may have been altered to reflect on disk case.

 @param CurrentDirectory Pointer to a string which will be referenced as part
        of this routine and updated to refer to the process current directory.
        This string will be NULL terminated within this routine.  The caller
        should free this with YoriLibFreeStringContents.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibGetCurrentDirectoryForDisplay(
    __out PYORI_STRING CurrentDirectory
    )
{
    if (YoriLibCurrentDirectoryForDisplay.LengthInChars == 0) {
        YORI_STRING LocalDir;
        YORI_STRING NewDisplayString;

        if (!YoriLibGetCurrentDirectory(&LocalDir)) {
            return FALSE;
        }

        if (!YoriLibGetOnDiskCaseForPath(&LocalDir, &NewDisplayString)) {
            YoriLibFreeStringContents(&LocalDir);
            return FALSE;
        }

        YoriLibFreeStringContents(&LocalDir);

        YoriLibFreeStringContents(&YoriLibCurrentDirectoryForDisplay);
        memcpy(&YoriLibCurrentDirectoryForDisplay, &NewDisplayString, sizeof(YORI_STRING));
    }

    YoriLibCloneString(CurrentDirectory, &YoriLibCurrentDirectoryForDisplay);
    return TRUE;
}

/**
 Set the current directory for the process.  This will also query the file
 system for on disk case, and internally remember the case corrected name.
 The process is expected to clean up the case corrected name by calling
 @ref YoriLibCleanupCurrentDirectory before terminating.

 @param NewCurrentDirectory Pointer to the directory to set the process
        current directory to.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibSetCurrentDirectory(
    __in PYORI_STRING NewCurrentDirectory
    )
{
    YORI_STRING NewDisplayString;
    LPTSTR NullTerminatedDirectory;
    BOOL AllocatedDirectory;
    BOOL Result;
    DWORD LastErr;

    if (!YoriLibGetOnDiskCaseForPath(NewCurrentDirectory, &NewDisplayString)) {
        return FALSE;
    }

    if (YoriLibIsStringNullTerminated(NewCurrentDirectory)) {
        NullTerminatedDirectory = NewCurrentDirectory->StartOfString;
        AllocatedDirectory = FALSE;
    } else {
        NullTerminatedDirectory = YoriLibCStringFromYoriString(NewCurrentDirectory);
        if (NullTerminatedDirectory == NULL) {
            YoriLibFreeStringContents(&NewDisplayString);
            return FALSE;
        }
        AllocatedDirectory = TRUE;
    }


    Result = SetCurrentDirectory(NullTerminatedDirectory);
    LastErr = GetLastError();
    if (AllocatedDirectory) {
        YoriLibDereference(NullTerminatedDirectory);
    }
    if (Result) {
        YoriLibFreeStringContents(&YoriLibCurrentDirectoryForDisplay);
        memcpy(&YoriLibCurrentDirectoryForDisplay, &NewDisplayString, sizeof(YORI_STRING));
    } else {
        YoriLibFreeStringContents(&NewDisplayString);
    }
    SetLastError(LastErr);

    return Result;
}

/**
 Change the current directory to an arbitrary path and modify any per-drive
 current directory if changing to a different drive.

 @param NewCurrentDirectory Pointer to the new directory.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriLibSetCurrentDirectorySaveDriveCurrentDirectory(
    __in PYORI_STRING NewCurrentDirectory
    )
{
    YORI_STRING OldCurrentDirectory;
    DWORD OldCurrentDirectoryLength;
    TCHAR NewDrive;
    TCHAR OldDrive;

    ASSERT(YoriLibIsStringNullTerminated(NewCurrentDirectory));

    OldCurrentDirectoryLength = GetCurrentDirectory(0, NULL);
    if (!YoriLibAllocateString(&OldCurrentDirectory, OldCurrentDirectoryLength)) {
        return FALSE;
    }

    OldCurrentDirectory.LengthInChars = GetCurrentDirectory(OldCurrentDirectory.LengthAllocated, OldCurrentDirectory.StartOfString);

    if (OldCurrentDirectory.LengthInChars == 0 ||
        OldCurrentDirectory.LengthInChars >= OldCurrentDirectory.LengthAllocated) {
        YoriLibFreeStringContents(&OldCurrentDirectory);
        return FALSE;
    }

    if (!YoriLibSetCurrentDirectory(NewCurrentDirectory)) {
        YoriLibFreeStringContents(&OldCurrentDirectory);
        return FALSE;
    }

    //
    //  Convert the first character to uppercase for comparison later.
    //

    OldDrive = OldCurrentDirectory.StartOfString[0];
    if (OldDrive >= 'a' && OldDrive <= 'z') {
        OldDrive = YoriLibUpcaseChar(OldDrive);
    }

    NewDrive = NewCurrentDirectory->StartOfString[0];
    if (NewDrive >= 'a' && NewDrive <= 'z') {
        NewDrive = YoriLibUpcaseChar(NewDrive);
    }

    //
    //  If the old current directory is drive letter based, preserve the old
    //  current directory in the environment.
    //

    if (OldDrive >= 'A' &&
        OldDrive <= 'Z' &&
        OldCurrentDirectory.StartOfString[1] == ':') {

        if (NewDrive != OldDrive) {
            YoriLibSetCurrentDirectoryOnDrive(OldDrive, &OldCurrentDirectory);
        }
    }

    YoriLibFreeStringContents(&OldCurrentDirectory);
    return TRUE;
}

/**
 Cleanup any allocated display current directory string.
 */
VOID
YoriLibCleanupCurrentDirectory(VOID)
{
    YoriLibFreeStringContents(&YoriLibCurrentDirectoryForDisplay);
}


// vim:sw=4:ts=4:et:
