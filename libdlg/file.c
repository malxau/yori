/**
 * @file libdlg/file.c
 *
 * Yori shell file selection dialog
 *
 * Copyright (c) 2019-2020 Malcolm J. Smith
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
#include <yoriwin.h>
#include <yoridlg.h>

/**
 A set of well known control IDs so the dialog can manipulate its elements.
 */
typedef enum _YORI_DLG_FILE_CONTROLS {
    YoriDlgFileControlFileText = 1,
    YoriDlgFileControlCurrentDirectory = 2,
    YoriDlgFileControlFileList = 3,
    YoriDlgFileControlDirectoryList = 4,
    YoriDlgFileFirstCustomCombo = 5
} YORI_DLG_FILE_CONTROLS;

/**
 State specific to a file dialog that is in operation.  Currently this is
 process global (can we really nest open dialogs anyway?)  But this should
 be moved to a per-window allocation.
 */
typedef struct _YORI_DLG_FILE_STATE {

    /**
     Specifies the current wildcard to apply to each directory.
     */
    YORI_STRING CurrentWildcard;

    /**
     Specifies the current directory within the dialog.  The process
     current directory is not changed within this module.
     */
    YORI_STRING CurrentDirectory;

    /**
     Specifies the file to return to the caller.  This is a full escaped
     path when populated.
     */
    YORI_STRING FileToReturn;

    /**
     Specifies the number of directories in the directory box.  Entries beyond
     this are interpreted as drives.
     */
    DWORD NumberDirectories;

} YORI_DLG_FILE_STATE, *PYORI_DLG_FILE_STATE;


/**
 The generic YoriLibGetFullPathNameRelTo fails if it is given a different
 drive but fails to specify an absolute path, because resolving it would
 require a current directory which was not specified to the function.  This
 wrapper resolves this case by checking if the drive specifies no path, and
 forcing it to be the root; and if it specifies a relative path, use the root
 of the drive as the current directory.

 @param PrimaryDirectory The directory to use as a "current" directory.

 @param FileName A file name, which may be fully specified or may be relative
        to PrimaryDirectory.

 @param ReturnEscapedPath If TRUE, the returned path is in \\?\ form.
        If FALSE, it is in regular Win32 form.

 @param Buffer On successful completion, updated to point to a newly
        allocated buffer containing the full path form of the file name.
        Note this is returned as a NULL terminated YORI_STRING, suitable
        for use in YORI_STRING or NULL terminated functions.

 @return TRUE to indicate success or FALSE to indicate failure.
 */
BOOL
YoriDlgFileGetFullPathNameRelativeTo(
    __in PYORI_STRING PrimaryDirectory,
    __in PYORI_STRING FileName,
    __in BOOL ReturnEscapedPath,
    __inout PYORI_STRING Buffer
    )
{
    if (FileName->LengthInChars >= 2 &&
        FileName->StartOfString[1] == ':' &&
        (FileName->LengthInChars == 2 || !YoriLibIsSep(FileName->StartOfString[2]))) {

        YORI_STRING DriveRoot;
        TCHAR DriveRootBuffer[4];

        YoriLibInitEmptyString(&DriveRoot);
        DriveRootBuffer[0] = FileName->StartOfString[0];
        DriveRootBuffer[1] = ':';
        DriveRootBuffer[2] = '\\';
        DriveRootBuffer[3] = '\0';
        DriveRoot.StartOfString = DriveRootBuffer;
        DriveRoot.LengthInChars = 3;

        if (FileName->LengthInChars == 2) {

            if (!YoriLibGetFullPathNameAlloc(&DriveRoot,
                                             ReturnEscapedPath,
                                             Buffer,
                                             NULL)) {

                return FALSE;
            }
        } else {
            YORI_STRING RelativeFileName;
            YoriLibInitEmptyString(&RelativeFileName);
            RelativeFileName.StartOfString = &FileName->StartOfString[2];
            RelativeFileName.LengthInChars = FileName->LengthInChars - 2;
            if (!YoriLibGetFullPathNameRelTo(&DriveRoot,
                                             &RelativeFileName,
                                             ReturnEscapedPath,
                                             Buffer,
                                             NULL)) {
                return FALSE;
            }
        }
    } else {

        if (!YoriLibGetFullPathNameRelTo(PrimaryDirectory,
                                         FileName,
                                         ReturnEscapedPath,
                                         Buffer,
                                         NULL)) {
            return FALSE;
        }
    }

    return TRUE;
}

VOID
YoriDlgFileRefreshView(
    __in PYORI_WIN_CTRL_HANDLE Dialog,
    __in PYORI_STRING Directory,
    __in PYORI_STRING Wildcard
    );

/**
 A callback invoked when the ok button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
YoriDlgFileOkButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PYORI_WIN_CTRL_HANDLE EditCtrl;
    YORI_STRING Text;
    YORI_STRING PathComponent;
    YORI_STRING FileComponent;
    YORI_STRING FullFilePath;
    YORI_STRING FullDirPath;
    PYORI_DLG_FILE_STATE State;
    YORI_ALLOC_SIZE_T Index;
    DWORD Attributes;
    BOOLEAN WildFound;

    //
    //  Cases:
    //  - Directory specified with wildcard
    //    (Change to directory, apply new wild)
    //  - Directory specified without wildcard
    //    (Change to directory, preserve old wild)
    //  - Wildcard specified
    //    (Update wildcard)
    //  - Path specified, can be relative or absolute.
    //    If parent exists and child doesn't, that indicates a name to use
    //

    Parent = YoriWinGetControlParent(Ctrl);
    EditCtrl = YoriWinFindControlById(Parent, YoriDlgFileControlFileText);
    ASSERT(EditCtrl != NULL);
    __analysis_assume(EditCtrl != NULL);
    YoriLibInitEmptyString(&Text);
    if (!YoriWinEditGetText(EditCtrl, &Text)) {
        return;
    }

    __analysis_assume(Text.StartOfString != NULL);

    State = YoriWinGetControlContext(Parent);
    YoriLibInitEmptyString(&PathComponent);
    YoriLibInitEmptyString(&FileComponent);

    //
    //  Split the string between text before the final seperator and text
    //  afterwards.
    //

    for (Index = Text.LengthInChars; Index > 0 ; Index--) {
        if (YoriLibIsSep(Text.StartOfString[Index - 1])) {
            PathComponent.StartOfString = Text.StartOfString;
            PathComponent.LengthInChars = Index - 1;
            FileComponent.StartOfString = &Text.StartOfString[Index];
            FileComponent.LengthInChars = Text.LengthInChars - Index;
            break;
        }

        //
        //  Check if it's X: (no backslash.)  It may be X:*.txt .
        //

        if (Index - 1 == 1 && Text.StartOfString[Index - 1] == ':') {
            PathComponent.StartOfString = Text.StartOfString;
            PathComponent.LengthInChars = Index;
            if (Index < Text.LengthInChars) {
                FileComponent.StartOfString = &Text.StartOfString[Index];
                FileComponent.LengthInChars = Text.LengthInChars - Index;
            }
            break;
        }
    }

    //
    //  If there's no seperator, treat all text as "afterwards."
    //

    if (PathComponent.StartOfString == NULL) {
        FileComponent.StartOfString = Text.StartOfString;
        FileComponent.LengthInChars = Text.LengthInChars;
    }

    //
    //  Look through the file part for a wildcard.  If one is found, then
    //  the display must be refreshed, possibly with a new directory 
    //  (in PathComponent) and a new wild (in FileComponent.)
    //

    WildFound = FALSE;
    for (Index = FileComponent.LengthInChars; Index > 0 ; Index--) {
        if (FileComponent.StartOfString[Index - 1] == '*' ||
            FileComponent.StartOfString[Index - 1] == '?') {

            WildFound = TRUE;
            break;
        }
    }

    YoriLibInitEmptyString(&FullFilePath);
    YoriLibInitEmptyString(&FullDirPath);

    //
    //  If there's a parent path, check to see that it exists and is a
    //  directory.  If not, then the specified file can't be returned to the
    //  caller.  If it is a directory, save off the full path so that this
    //  can be used to refresh the dialog from.
    //

    if (PathComponent.StartOfString != NULL) {

        if (!YoriDlgFileGetFullPathNameRelativeTo(&State->CurrentDirectory,
                                                  &PathComponent,
                                                  TRUE,
                                                  &FullDirPath)) {
            YoriLibFreeStringContents(&Text);
            return;
        }

        Attributes = GetFileAttributes(FullDirPath.StartOfString);

        if (Attributes == (DWORD)-1 ||
            (Attributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {

            YORI_STRING Title;
            YORI_STRING DialogText;
            YORI_STRING ButtonText;

            YoriLibConstantString(&DialogText, _T("Specified directory not found"));
            YoriLibConstantString(&Title, _T("Error"));
            YoriLibConstantString(&ButtonText, _T("Ok"));

            YoriDlgMessageBox(YoriWinGetWindowManagerHandle(Parent),
                              &Title,
                              &DialogText,
                              1,
                              &ButtonText,
                              0,
                              0);

            YoriLibFreeStringContents(&Text);
            YoriLibFreeStringContents(&FullDirPath);
            return;
        }

        PathComponent.StartOfString = FullDirPath.StartOfString;
        PathComponent.LengthInChars = FullDirPath.LengthInChars;
    }

    //
    //  If no wild is found, the string needs to be checked to see whether
    //  it refers to a directory or a file.  If it's a directory the display
    //  needs to be updated, if it's a file or nonexistent, the dialog can
    //  terminate and return the result.
    //

    if (!WildFound) {
        if (!YoriDlgFileGetFullPathNameRelativeTo(&State->CurrentDirectory,
                                                  &Text,
                                                  TRUE,
                                                  &FullFilePath)) {
            YoriLibFreeStringContents(&Text);
            YoriLibFreeStringContents(&FullDirPath);
            return;
        }

        Attributes = GetFileAttributes(FullFilePath.StartOfString);

        // Object not found - return as a file?
        if (Attributes == (DWORD)-1) {
            YoriLibInitEmptyString(&PathComponent);
            YoriLibInitEmptyString(&FileComponent);
        } else if (Attributes & FILE_ATTRIBUTE_DIRECTORY) {
            PathComponent.StartOfString = FullFilePath.StartOfString;
            PathComponent.LengthInChars = FullFilePath.LengthInChars;
            YoriLibInitEmptyString(&FileComponent);
        } else {
            YoriLibInitEmptyString(&PathComponent);
            YoriLibInitEmptyString(&FileComponent);
        }
    }

    //
    //  If there is a new directory or a new wildcard, refresh the view with
    //  those and complete processing.
    //

    if (PathComponent.StartOfString != NULL || FileComponent.StartOfString != NULL) {

        if (PathComponent.StartOfString == NULL) {
            PathComponent.StartOfString = State->CurrentDirectory.StartOfString;
            PathComponent.LengthInChars = State->CurrentDirectory.LengthInChars;
        }

        if (FileComponent.StartOfString == NULL) {
            FileComponent.StartOfString = State->CurrentWildcard.StartOfString;
            FileComponent.LengthInChars = State->CurrentWildcard.LengthInChars;
        }

        YoriDlgFileRefreshView(Parent, &PathComponent, &FileComponent);
        YoriWinEditSetText(EditCtrl, &FileComponent);
        YoriWinEditSetSelectionRange(EditCtrl, 0, FileComponent.LengthInChars);
        YoriLibFreeStringContents(&Text);
        YoriLibFreeStringContents(&FullFilePath);
        YoriLibFreeStringContents(&FullDirPath);
        return;
    }

    YoriLibFreeStringContents(&Text);
    YoriLibFreeStringContents(&FullDirPath);

    YoriLibFreeStringContents(&State->FileToReturn);
    memcpy(&State->FileToReturn, &FullFilePath, sizeof(YORI_STRING));

    YoriWinCloseWindow(Parent, TRUE);
}

/**
 A callback invoked when the cancel button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
YoriDlgFileCancelButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    YoriWinCloseWindow(Parent, FALSE);
}

/**
 A callback invoked when the selection within the file list changes.

 @param Ctrl Pointer to the list whose selection changed.
 */
VOID
YoriDlgFileFileSelectionChanged(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    YORI_ALLOC_SIZE_T ActiveOption;
    YORI_STRING String;
    PYORI_WIN_CTRL_HANDLE Parent;
    PYORI_WIN_CTRL_HANDLE EditCtrl;

    if (!YoriWinListGetActiveOption(Ctrl, &ActiveOption)) {
        return;
    }

    Parent = YoriWinGetControlParent(Ctrl);
    YoriLibInitEmptyString(&String);
    if (!YoriWinListGetItemText(Ctrl, ActiveOption, &String)) {
        return;
    }

    EditCtrl = YoriWinFindControlById(Parent, YoriDlgFileControlFileText);
    ASSERT(EditCtrl != NULL);
    __analysis_assume(EditCtrl != NULL);

    YoriWinEditSetText(EditCtrl, &String);
    YoriWinEditSetSelectionRange(EditCtrl, 0, String.LengthInChars);
    YoriLibFreeStringContents(&String);
}

/**
 A callback invoked when the selection within the directory list changes.

 @param Ctrl Pointer to the list whose selection changed.
 */
VOID
YoriDlgFileDirectorySelectionChanged(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    YORI_ALLOC_SIZE_T ActiveOption;
    YORI_STRING String;
    PYORI_WIN_CTRL_HANDLE Parent;
    PYORI_WIN_CTRL_HANDLE EditCtrl;
    PYORI_DLG_FILE_STATE State;

    if (!YoriWinListGetActiveOption(Ctrl, &ActiveOption)) {
        return;
    }

    Parent = YoriWinGetControlParent(Ctrl);
    State = YoriWinGetControlContext(Parent);
    YoriLibInitEmptyString(&String);
    if (!YoriWinListGetItemText(Ctrl, ActiveOption, &String)) {
        return;
    }

    //
    //  If the user has selected an item beyond the set of directories, this
    //  is a drive.  The drive string follows a well known format of [-X-],
    //  which needs to be transposed to X: .
    //

    if (ActiveOption >= State->NumberDirectories) {
        String.StartOfString[0] = String.StartOfString[2];
        String.StartOfString[1] = ':';
        String.StartOfString[2] = '\0';
        String.LengthInChars = 2;
    }

    EditCtrl = YoriWinFindControlById(Parent, YoriDlgFileControlFileText);
    ASSERT(EditCtrl != NULL);
    __analysis_assume(EditCtrl != NULL);

    YoriWinEditSetText(EditCtrl, &String);
    YoriWinEditSetSelectionRange(EditCtrl, 0, String.LengthInChars);
    YoriLibFreeStringContents(&String);
}

/**
 A callback that is invoked when a file is found that matches a search criteria
 and should be added to the file list.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.  This can be NULL if the file was
        not found from enumeration, since the file may not be a file system
        object (ie., it may be a device.)

 @param Depth Indicates the recursion depth.

 @param Context Pointer to a the control to add the found files to.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL 
YoriDlgFileFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    YORI_STRING FileNameOnly;
    PYORI_STRING_ARRAY StringArray;

    UNREFERENCED_PARAMETER(FilePath);
    UNREFERENCED_PARAMETER(Depth);

    if (FileInfo == NULL) {
        return TRUE;
    }

    StringArray = (PYORI_STRING_ARRAY)Context;
    YoriLibConstantString(&FileNameOnly, FileInfo->cFileName);

    YoriStringArrayAddItems(StringArray, &FileNameOnly, 1);
    return TRUE;
}

/**
 A callback that is invoked when a directory is found that matches a search
 criteria and should be added to the directory list.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the directory.  This can be NULL if the
        file was not found from enumeration, since the file may not be a file
        system object (ie., it may be a device.)

 @param Depth Indicates the recursion depth.

 @param Context Pointer to a the control to add the found directories to.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL 
YoriDlgFileDirFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    YORI_STRING FileNameOnly;
    PYORI_STRING_ARRAY StringArray;

    UNREFERENCED_PARAMETER(FilePath);
    UNREFERENCED_PARAMETER(Depth);

    if (FileInfo == NULL) {
        return TRUE;
    }

    //
    //  Don't include "." since it's a no-op
    //

    if (FileInfo->cFileName[0] == '.' && FileInfo->cFileName[1] == '\0') {
        return TRUE;
    }

    StringArray = (PYORI_STRING_ARRAY)Context;
    YoriLibConstantString(&FileNameOnly, FileInfo->cFileName);

    YoriStringArrayAddItems(StringArray, &FileNameOnly, 1);
    return TRUE;
}

/**
 Refresh the dialog by searching a specified directory for files and
 subdirectories, populating the UI elements with those items, and updating
 the current directory string within the dialog.

 @param Dialog Pointer to the dialog window.

 @param Directory Pointer to a string containing the directory to search.

 @param Wildcard Pointer to a filter string to determine the files to find
        within the directory.
 */
VOID
YoriDlgFileRefreshView(
    __in PYORI_WIN_CTRL_HANDLE Dialog,
    __in PYORI_STRING Directory,
    __in PYORI_STRING Wildcard
    )
{
    YORI_STRING FullDir;
    YORI_STRING UnescapedPath;
    YORI_STRING SearchString;
    PYORI_WIN_CTRL_HANDLE CurDirLabel;
    PYORI_WIN_CTRL_HANDLE DirList;
    PYORI_WIN_CTRL_HANDLE FileList;
    PYORI_DLG_FILE_STATE State;
    YORI_STRING DriveDisplay;
    TCHAR DriveProbeString[sizeof("A:\\")];
    TCHAR DriveDisplayString[sizeof("[-A-]")];
    UINT DriveProbeResult;
    YORI_STRING_ARRAY NewListItems;

    if (!YoriLibUserToSingleFilePath(Directory, TRUE, &FullDir)) {
        return;
    }

    YoriLibInitEmptyString(&UnescapedPath);
    if (!YoriLibUnescapePath(&FullDir, &UnescapedPath)) {
        YoriLibFreeStringContents(&FullDir);
        return;
    }

    CurDirLabel = YoriWinFindControlById(Dialog, YoriDlgFileControlCurrentDirectory);
    ASSERT(CurDirLabel != NULL);
    __analysis_assume(CurDirLabel != NULL);
    DirList = YoriWinFindControlById(Dialog, YoriDlgFileControlDirectoryList);
    ASSERT(DirList != NULL);
    __analysis_assume(DirList != NULL);
    FileList = YoriWinFindControlById(Dialog, YoriDlgFileControlFileList);
    ASSERT(FileList != NULL);
    __analysis_assume(FileList != NULL);

    YoriWinLabelSetCaption(CurDirLabel, &UnescapedPath);

    YoriLibFreeStringContents(&UnescapedPath);

    State = YoriWinGetControlContext(Dialog);
    YoriLibFreeStringContents(&State->CurrentDirectory);
    memcpy(&State->CurrentDirectory, &FullDir, sizeof(YORI_STRING));

    //
    //  Save the wildcard for use in any later directory navigation, if it
    //  has changed.
    //

    if (Wildcard->StartOfString != State->CurrentWildcard.StartOfString ||
        Wildcard->LengthInChars != State->CurrentWildcard.LengthInChars) {

        YoriLibInitEmptyString(&SearchString);
        if (!YoriLibCopyString(&SearchString, Wildcard)) {
            return;
        }

        YoriLibFreeStringContents(&State->CurrentWildcard);
        memcpy(&State->CurrentWildcard, &SearchString, sizeof(YORI_STRING));
    }

    YoriLibInitEmptyString(&SearchString);

    //
    //  Populate file list.  This should match the specified wildcard.
    //

    YoriLibYPrintf(&SearchString, _T("%y\\%y"), &FullDir, &State->CurrentWildcard);
    if (SearchString.StartOfString == NULL) {
        return;
    }

    YoriWinListClearAllItems(FileList);
    YoriStringArrayInitialize(&NewListItems);
    YoriLibForEachFile(&SearchString, YORILIB_ENUM_RETURN_FILES | YORILIB_ENUM_BASIC_EXPANSION, 0, YoriDlgFileFileFoundCallback, NULL, &NewListItems);
    YoriLibSortStringArray(NewListItems.Items, NewListItems.Count);
    YoriWinListAddItems(FileList, NewListItems.Items, NewListItems.Count);
    YoriStringArrayCleanup(&NewListItems);


    //
    //  Populate directory list.  This matches all directories.
    //

    YoriLibYPrintf(&SearchString, _T("%y\\*"), &FullDir);
    if (SearchString.StartOfString == NULL) {
        return;
    }

    YoriWinListClearAllItems(DirList);
    YoriLibForEachFile(&SearchString, YORILIB_ENUM_RETURN_DIRECTORIES | YORILIB_ENUM_INCLUDE_DOTFILES | YORILIB_ENUM_BASIC_EXPANSION, 0, YoriDlgFileDirFoundCallback, NULL, &NewListItems);
    YoriLibSortStringArray(NewListItems.Items, NewListItems.Count);
    YoriWinListAddItems(DirList, NewListItems.Items, NewListItems.Count);
    YoriStringArrayCleanup(&NewListItems);
    State->NumberDirectories = YoriWinListGetItemCount(DirList);

    //
    //  Add drives to the directory list.
    //

    YoriLibInitEmptyString(&DriveDisplay);
    DriveDisplayString[0] = '[';
    DriveDisplayString[1] = '-';
    DriveDisplayString[3] = '-';
    DriveDisplayString[4] = ']';
    DriveDisplay.StartOfString = DriveDisplayString;
    DriveDisplay.LengthInChars = sizeof("[-A-]") - 1;

    DriveProbeString[1] = ':';
    DriveProbeString[2] = '\\';
    DriveProbeString[3] = '\0';

    for (DriveProbeString[0] = 'A'; DriveProbeString[0] <= 'Z'; DriveProbeString[0]++) {
        DriveProbeResult = GetDriveType(DriveProbeString);
        if (DriveProbeResult != DRIVE_UNKNOWN &&
            DriveProbeResult != DRIVE_NO_ROOT_DIR) {

            DriveDisplayString[2] = DriveProbeString[0];
            YoriWinListAddItems(DirList, &DriveDisplay, 1);
        }
    }

    YoriLibFreeStringContents(&SearchString);
}

/**
 Display a file selection dialog box.

 @param WinMgrHandle Pointer to the window manager.

 @param Title The string to display in the title of the dialog.

 @param OptionCount The number of custom pull down list controls at the bottom
        of the dialog.

 @param Options Pointer to an array of structures describing each custom pull
        down list at the bottom of the dialog.

 @param Text On input, specifies an initialized string.  On output, populated
        with the path to the file that the user entered.

 @return TRUE to indicate that the user successfully selected a file.  FALSE
         to indicate a failure or the user cancelling the operation.
 */
__success(return)
BOOLEAN
YoriDlgFile(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_STRING Title,
    __in DWORD OptionCount,
    __in_opt PYORI_DLG_FILE_CUSTOM_OPTION Options,
    __inout PYORI_STRING Text
    )
{
    PYORI_WIN_WINDOW_HANDLE Parent;
    COORD WindowSize;
    SMALL_RECT Area;
    YORI_STRING Caption;
    YORI_STRING Wildcard;
    WORD ButtonWidth;
    PYORI_WIN_CTRL_HANDLE Ctrl;
    PYORI_WIN_CTRL_HANDLE Edit;
    DWORD_PTR Result;
    COORD WinMgrSize;
    YORI_DLG_FILE_STATE State;
    DWORD Index;
    DWORD LongestOptionDescription;

    if (!YoriWinGetWinMgrDimensions(WinMgrHandle, &WinMgrSize)) {
        WinMgrSize.X = 75;
        WinMgrSize.Y = 20;
    } else {
        WinMgrSize.X = (SHORT)(WinMgrSize.X * 9 / 10);
        if (WinMgrSize.X < 50) {
            WinMgrSize.X = 50;
        }

        //
        //  If the window height is less than 30 rows, use 80%.  Otherwise,
        //  use 66%.  To prevent the discontinuity, if 80% of 30 rows is
        //  larger than 66% of actual rows, use that.
        //

        if (WinMgrSize.Y < 30) {
            WinMgrSize.Y = (SHORT)(WinMgrSize.Y * 4 / 5);
        } else {
            WinMgrSize.Y = (SHORT)(WinMgrSize.Y * 2 / 3);
            if ((SHORT)(30 * 3 / 4) > WinMgrSize.Y) {
                WinMgrSize.Y = (SHORT)(30 * 4 / 5);
            }
        }
    }

    if ((WORD)WinMgrSize.Y < 13 + OptionCount) {
        WinMgrSize.Y = (SHORT)(13 + OptionCount);
    }

    YoriLibInitEmptyString(&State.CurrentDirectory);
    YoriLibInitEmptyString(&State.CurrentWildcard);
    YoriLibInitEmptyString(&State.FileToReturn);
    if (!YoriWinCreateWindow(WinMgrHandle, 50, (WORD)(13 + OptionCount), WinMgrSize.X, WinMgrSize.Y, YORI_WIN_WINDOW_STYLE_BORDER_SINGLE | YORI_WIN_WINDOW_STYLE_SHADOW_SOLID, Title, &Parent)) {
        return FALSE;
    }

    YoriWinSetControlContext(Parent, &State);
    YoriWinGetClientSize(Parent, &WindowSize);

    YoriLibConstantString(&Caption, _T("File &Name:"));

    Area.Left = 1;
    Area.Top = 1;
    Area.Right = (WORD)(Area.Left + Caption.LengthInChars - 1);
    Area.Bottom = 1;

    Ctrl = YoriWinLabelCreate(Parent, &Area, &Caption, 0);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Area.Top = 0;
    Area.Bottom = 2;
    Area.Left = (WORD)(Area.Right + 1);
    Area.Right = (WORD)(WindowSize.X - 2);

    YoriLibConstantString(&Caption, _T(""));

    Edit = YoriWinEditCreate(Parent, &Area, &Caption, 0);
    if (Edit == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    YoriWinSetControlId(Edit, YoriDlgFileControlFileText);

    Area.Top = (WORD)(Area.Bottom + 1);
    Area.Bottom = Area.Top;
    Area.Left = 1;
    Area.Right = (WORD)(WindowSize.X - 2);

    Ctrl = YoriWinLabelCreate(Parent, &Area, &Caption, YORI_WIN_LABEL_NO_ACCELERATOR);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    YoriWinSetControlId(Ctrl, YoriDlgFileControlCurrentDirectory);

    YoriLibConstantString(&Caption, _T("&Files:"));

    Area.Top = (WORD)(Area.Bottom + 1);
    Area.Bottom = Area.Top;
    Area.Left = 3;
    Area.Right = (WORD)(Area.Left + Caption.LengthInChars - 1);

    Ctrl = YoriWinLabelCreate(Parent, &Area, &Caption, 0);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Area.Top = (WORD)(Area.Bottom + 1);
    Area.Left = 1;
    Area.Bottom = (WORD)(WindowSize.Y - OptionCount - 4);
    Area.Right = (WORD)(WindowSize.X / 2 - 1);

    Ctrl = YoriWinListCreate(Parent, &Area, YORI_WIN_LIST_STYLE_VSCROLLBAR | YORI_WIN_LIST_STYLE_DESELECT_ON_LOSE_FOCUS | YORI_WIN_LIST_STYLE_AUTO_HSCROLLBAR);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    YoriWinControlSetFocusOnMouseClick(Ctrl, FALSE);
    YoriWinSetControlId(Ctrl, YoriDlgFileControlFileList);
    YoriWinListSetSelectionNotifyCallback(Ctrl, YoriDlgFileFileSelectionChanged);

    YoriLibConstantString(&Caption, _T("&Directories:"));

    Area.Top = (WORD)(Area.Top - 1);
    Area.Left = (WORD)(Area.Right + 4);
    Area.Right = (WORD)(Area.Left + Caption.LengthInChars - 1);
    Area.Bottom = Area.Top;

    Ctrl = YoriWinLabelCreate(Parent, &Area, &Caption, 0);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Area.Top = (WORD)(Area.Bottom + 1);
    Area.Left = (WORD)(Area.Left - 2);
    Area.Bottom = (WORD)(WindowSize.Y - OptionCount - 4);
    Area.Right = (WORD)(WindowSize.X - 2);

    Ctrl = YoriWinListCreate(Parent, &Area, YORI_WIN_LIST_STYLE_VSCROLLBAR | YORI_WIN_LIST_STYLE_DESELECT_ON_LOSE_FOCUS | YORI_WIN_LIST_STYLE_AUTO_HSCROLLBAR);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    YoriWinControlSetFocusOnMouseClick(Ctrl, FALSE);
    YoriWinSetControlId(Ctrl, YoriDlgFileControlDirectoryList);
    YoriWinListSetSelectionNotifyCallback(Ctrl, YoriDlgFileDirectorySelectionChanged);

    LongestOptionDescription = 0;
    for (Index = 0; Index < OptionCount; Index++) {
        if (Options[Index].Description.LengthInChars > LongestOptionDescription) {
            LongestOptionDescription = Options[Index].Description.LengthInChars;
        }
    }

    if (LongestOptionDescription > (DWORD)(WindowSize.X - 10)) {
        LongestOptionDescription = (DWORD)(WindowSize.X - 10);
    }

    for (Index = 0; Index < OptionCount; Index++) {
        Area.Left = 1;
        Area.Right = (SHORT)(Area.Left + LongestOptionDescription);
        Area.Top = (SHORT)(WindowSize.Y - 3 - OptionCount + Index);
        Area.Bottom = Area.Top;

        Ctrl = YoriWinLabelCreate(Parent, &Area, &Options[Index].Description, 0);
        if (Ctrl == NULL) {
            YoriWinDestroyWindow(Parent);
            return FALSE;
        }

        Area.Left = (SHORT)(LongestOptionDescription + 2);
        Area.Right = (SHORT)(WindowSize.X - 2);

        ButtonWidth = 5;
        if (Options[Index].ValueCount < ButtonWidth) {
            ButtonWidth = (WORD)Options[Index].ValueCount;
        }

        Ctrl = YoriWinComboCreate(Parent, &Area, ButtonWidth, &Options[Index].Values[0].ValueText, 0, NULL);
        if (Ctrl == NULL) {
            YoriWinDestroyWindow(Parent);
            return FALSE;
        }

        YoriWinSetControlId(Ctrl, YoriDlgFileFirstCustomCombo + Index);

        if (!YoriWinComboAddItems(Ctrl, (PYORI_STRING)Options[Index].Values, Options[Index].ValueCount)) {
            YoriWinDestroyWindow(Parent);
            return FALSE;
        }
        YoriWinComboSetActiveOption(Ctrl, Options[Index].SelectedValue);
    }

    ButtonWidth = (WORD)(8);

    Area.Top = (SHORT)(WindowSize.Y - 3);
    Area.Bottom = (SHORT)(Area.Top + 2);

    YoriLibConstantString(&Caption, _T("&Ok"));

    Area.Left = (SHORT)(1);
    Area.Right = (WORD)(Area.Left + 1 + ButtonWidth);

    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, YORI_WIN_BUTTON_STYLE_DEFAULT, YoriDlgFileOkButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Area.Left = (WORD)(Area.Left + ButtonWidth + 3);
    Area.Right = (WORD)(Area.Right + ButtonWidth + 3);

    YoriLibConstantString(&Caption, _T("&Cancel"));
    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, YORI_WIN_BUTTON_STYLE_CANCEL, YoriDlgFileCancelButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    YoriLibConstantString(&Caption, _T("."));
    YoriLibConstantString(&Wildcard, _T("*"));
    YoriDlgFileRefreshView(Parent, &Caption, &Wildcard);

    Result = FALSE;
    if (!YoriWinProcessInputForWindow(Parent, &Result)) {
        Result = FALSE;
    }

    if (Result) {
        if (Text->LengthAllocated >= State.FileToReturn.LengthInChars + 1) {
            memcpy(Text->StartOfString, State.FileToReturn.StartOfString, State.FileToReturn.LengthInChars * sizeof(TCHAR));
            Text->StartOfString[State.FileToReturn.LengthInChars] = '\0';
            Text->LengthInChars = State.FileToReturn.LengthInChars;
        } else {
            YoriLibFreeStringContents(Text);
            memcpy(Text, &State.FileToReturn, sizeof(YORI_STRING));
            YoriLibInitEmptyString(&State.FileToReturn);
        }

        for (Index = 0; Index < OptionCount; Index++) {
            Ctrl = YoriWinFindControlById(Parent, YoriDlgFileFirstCustomCombo + Index);
            ASSERT(Ctrl != NULL);
            __analysis_assume(Ctrl != NULL);
            YoriWinComboGetActiveOption(Ctrl, &Options[Index].SelectedValue);
        }
    }

    YoriLibFreeStringContents(&State.CurrentDirectory);
    YoriLibFreeStringContents(&State.CurrentWildcard);
    YoriLibFreeStringContents(&State.FileToReturn);
    YoriWinDestroyWindow(Parent);
    return (BOOLEAN)Result;
}

// vim:sw=4:ts=4:et:
