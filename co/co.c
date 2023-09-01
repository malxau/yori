/**
 * @file co/co.c
 *
 * Yori shell mini file manager
 *
 * Copyright (c) 2019-2023 Malcolm J. Smith
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
 The set of control identifiers allocated within this program.
 */
typedef enum _CO_CONTROLS {
    CoCtrlList = 1,
    CoCtrlExit = 2,
    CoCtrlChangeDir = 3,
    CoCtrlDelete = 4,
    CoCtrlCopy = 5,
    CoCtrlMove = 6,
    CoCtrlSortLabel = 7,
    CoCtrlSortCombo = 8,
} CO_CONTROLS;

/**
 Help text to display to the user.
 */
const
CHAR strCoHelpText[] =
        "\n"
        "Displays file manager.\n"
        "\n"
        "CO [-license]\n";

/**
 Display usage text to the user.
 */
BOOL
CoHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Co %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strCoHelpText);
    return TRUE;
}

/**
 Information about a single found file.
 */
typedef struct _CO_FOUND_FILE {

    /**
     The found file link within the list of found files.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     The string to display in the list control.  The buffer for this string
     follows this structure.
     */
    YORI_STRING DisplayName;

    /**
     The string to specify a full path to this file.  The buffer for this
     string follows this structure.
     */
    YORI_STRING FullFilePath;

    /**
     The size of the file, in bytes.
     */
    LARGE_INTEGER FileSize;

    /**
     The time that the file was last modified.
     */
    LARGE_INTEGER WriteTime;

    /**
     TRUE if the object is a directory, FALSE if it is a file.
     */
    BOOLEAN IsDirectory;

} CO_FOUND_FILE, *PCO_FOUND_FILE;

/**
 The set of sort orders that can be applied.
 */
typedef enum _CO_SORT_TYPE {
    CoSortByName = 0,
    CoSortBySize = 1,
    CoSortByDate = 2,
    CoSortBeyondMaximum = 3
} CO_SORT_TYPE;

/**
 A context that records files found and being operated on in the current
 window.
 */
typedef struct _CO_CONTEXT {

    /**
     A linked list of the files that have been found.
     */
    YORI_LIST_ENTRY FilesFound;

    /**
     The number of files that have been found.
     */
    DWORD FilesFoundCount;

    /**
     The sort order currently being applied.  Note this is not reset in
     CoFreeContext, because it needs to be preserved across repopulation.
     */
    CO_SORT_TYPE SortType;

    /**
     Pointer to the list control.
     */
    PYORI_WIN_CTRL_HANDLE List;

    /**
     The same list as above, arranged into a flat array form to match the
     addressing of the list control.
     */
    PCO_FOUND_FILE* FileArray;

    /**
     Pointer to the window manager.
     */
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgr;

    /**
     The current directory for the application.
     */
    YORI_STRING CurrentDirectory;
} CO_CONTEXT, *PCO_CONTEXT;



/**
 Free all found files in the list.

 @param CoContext Pointer to the context of found files to free.
 */
VOID
CoFreeFileList(
    __in PCO_CONTEXT CoContext
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PCO_FOUND_FILE FoundFile;

    ListEntry = NULL;
    ListEntry = YoriLibGetNextListEntry(&CoContext->FilesFound, ListEntry);
    while (ListEntry != NULL) {
        FoundFile = CONTAINING_RECORD(ListEntry, CO_FOUND_FILE, ListEntry);
        ListEntry = YoriLibGetNextListEntry(&CoContext->FilesFound, ListEntry);

        YoriLibRemoveListItem(&FoundFile->ListEntry);
        YoriLibFreeStringContents(&FoundFile->DisplayName);
        YoriLibFreeStringContents(&FoundFile->FullFilePath);
        YoriLibDereference(FoundFile);
    }

    if (CoContext->FileArray != NULL) {
        YoriLibFree(CoContext->FileArray);
        CoContext->FileArray = NULL;
    }

    CoContext->FilesFoundCount = 0;
}

/**
 Free all allocations in the context.

 @param CoContext Pointer to the context of found files to free.
 */
VOID
CoFreeContext(
    __in PCO_CONTEXT CoContext
    )
{
    CoFreeFileList(CoContext);
    YoriLibFreeStringContents(&CoContext->CurrentDirectory);
}

/**
 A callback that is invoked when a file is found that should be added to the
 list.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Specifies the recursion depth.  Ignored in this application.

 @param Context Pointer to the co context specifying the list to populate
        with found files.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
CoFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    PCO_CONTEXT CoContext = (PCO_CONTEXT)Context;
    PCO_FOUND_FILE FoundFile;
    DWORD DisplayNameLength;

    UNREFERENCED_PARAMETER(Depth);

    DisplayNameLength = _tcslen(FileInfo->cFileName);
    FoundFile = YoriLibReferencedMalloc(sizeof(CO_FOUND_FILE) + (DisplayNameLength + 1 + FilePath->LengthInChars + 1) * sizeof(TCHAR));
    if (FoundFile == NULL) {
        return FALSE;
    }

    YoriLibInitEmptyString(&FoundFile->DisplayName);
    YoriLibInitEmptyString(&FoundFile->FullFilePath);

    YoriLibReference(FoundFile);
    FoundFile->DisplayName.MemoryToFree = FoundFile;
    FoundFile->DisplayName.StartOfString = (LPTSTR)(FoundFile + 1);
    FoundFile->DisplayName.LengthInChars = DisplayNameLength;
    FoundFile->DisplayName.LengthAllocated = DisplayNameLength + 1;

    memcpy(FoundFile->DisplayName.StartOfString, FileInfo->cFileName, DisplayNameLength * sizeof(TCHAR));
    FoundFile->DisplayName.StartOfString[FoundFile->DisplayName.LengthInChars] = '\0';

    YoriLibReference(FoundFile);
    FoundFile->FullFilePath.MemoryToFree = FoundFile;
    FoundFile->FullFilePath.StartOfString = FoundFile->DisplayName.StartOfString + FoundFile->DisplayName.LengthAllocated;
    FoundFile->FullFilePath.LengthInChars = FilePath->LengthInChars;
    FoundFile->FullFilePath.LengthAllocated = FilePath->LengthInChars + 1;

    memcpy(FoundFile->FullFilePath.StartOfString, FilePath->StartOfString, FilePath->LengthInChars * sizeof(TCHAR));
    FoundFile->FullFilePath.StartOfString[FoundFile->FullFilePath.LengthInChars] = '\0';

    FoundFile->FileSize.HighPart = FileInfo->nFileSizeHigh;
    FoundFile->FileSize.LowPart = FileInfo->nFileSizeLow;
    FoundFile->WriteTime.HighPart = FileInfo->ftLastWriteTime.dwHighDateTime;
    FoundFile->WriteTime.LowPart = FileInfo->ftLastWriteTime.dwLowDateTime;

    if (FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        FoundFile->IsDirectory = TRUE;
    } else {
        FoundFile->IsDirectory = FALSE;
    }

    YoriLibAppendList(&CoContext->FilesFound, &FoundFile->ListEntry);
    CoContext->FilesFoundCount++;
    return TRUE;
}

/**
 Populate in memory structures and the UI list with found files.

 @param CoContext Pointer to the context to populate with found files.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
CoPopulateList(
    __in PCO_CONTEXT CoContext
    )
{
    YORI_STRING FileSpec;
    PYORI_STRING DisplayArray;
    DWORD Index;
    DWORD SubIndex;
    DWORD BestIndex;
    PYORI_LIST_ENTRY ListEntry;
    PCO_FOUND_FILE FoundFile;
    PCO_FOUND_FILE CompareFile;
    PCO_FOUND_FILE BestFile;

    YoriLibInitEmptyString(&FileSpec);
    YoriLibYPrintf(&FileSpec, _T("%y\\*"), &CoContext->CurrentDirectory);
    YoriLibForEachFile(&FileSpec, YORILIB_FILEENUM_BASIC_EXPANSION | YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_RETURN_DIRECTORIES | YORILIB_FILEENUM_INCLUDE_DOTFILES, 0, CoFileFoundCallback, NULL, CoContext);
    YoriLibFreeStringContents(&FileSpec);

    if (CoContext->FilesFoundCount == 0) {
        return TRUE;
    }

    DisplayArray = YoriLibMalloc(sizeof(YORI_STRING) * CoContext->FilesFoundCount);
    if (DisplayArray == NULL) {
        return FALSE;
    }

    CoContext->FileArray = YoriLibMalloc(sizeof(PCO_FOUND_FILE) * CoContext->FilesFoundCount);
    if (CoContext->FileArray == NULL) {
        YoriLibDereference(DisplayArray);
        return FALSE;
    }

    //
    //  Arrange the files into a flat array
    //

    ListEntry = NULL;
    for (Index = 0; Index < CoContext->FilesFoundCount; Index++) {
        ListEntry = YoriLibGetNextListEntry(&CoContext->FilesFound, ListEntry);
        FoundFile = CONTAINING_RECORD(ListEntry, CO_FOUND_FILE, ListEntry);
        CoContext->FileArray[Index] = FoundFile;
    }

    //
    //  Sort the array based on the selected sort criteria
    //

    for (Index = 0; Index < CoContext->FilesFoundCount; Index++) {
        FoundFile = CoContext->FileArray[Index];
        BestFile = FoundFile;
        BestIndex = Index;
        for (SubIndex = Index + 1; SubIndex < CoContext->FilesFoundCount; SubIndex++) {
            CompareFile = CoContext->FileArray[SubIndex];
            if (CoContext->SortType == CoSortByName) {
                if (YoriLibCompareStringInsensitive(&CompareFile->DisplayName, &BestFile->DisplayName) < 0) {
                    BestFile = CompareFile;
                    BestIndex = SubIndex;
                }
            } else if (CoContext->SortType == CoSortBySize) {
                if (CompareFile->FileSize.QuadPart < BestFile->FileSize.QuadPart) {
                    BestFile = CompareFile;
                    BestIndex = SubIndex;
                }
            } else if (CoContext->SortType == CoSortByDate) {
                if (CompareFile->WriteTime.QuadPart < BestFile->WriteTime.QuadPart) {
                    BestFile = CompareFile;
                    BestIndex = SubIndex;
                }
            }
        }

        if (BestIndex != Index) {
            CoContext->FileArray[BestIndex] = CoContext->FileArray[Index];
            CoContext->FileArray[Index] = BestFile;
        }
    }

    //
    //  Generate the display array string based on the result of the sort
    //

    for (Index = 0; Index < CoContext->FilesFoundCount; Index++) {
        memcpy(&DisplayArray[Index], &CoContext->FileArray[Index]->DisplayName, sizeof(YORI_STRING));
    }

    //
    //  Populate the list with the result
    //

    if (!YoriWinListAddItems(CoContext->List, DisplayArray, CoContext->FilesFoundCount)) {
        YoriLibDereference(DisplayArray);
        return FALSE;
    }

    YoriLibFree(DisplayArray);
    return TRUE;
}

/**
 Clear the contents of the list and start over.  This isn't a great user
 experience because of how it affects the view region.  We'll want something
 better.

 @param CoContext Pointer to context about files to display and the list to
        display them in.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
CoRepopulateList(
    __in PCO_CONTEXT CoContext
    )
{
    CoFreeFileList(CoContext);
    YoriWinListClearAllItems(CoContext->List);
    return CoPopulateList(CoContext);
}

/**
 Verify that a file is selected.  If no file is selected, display a dialog
 letting the user know.

 @param CoContext Pointer to the context describing selected items.

 @return TRUE to indicate that a file is selected and the file operation can
         continue, FALSE to indicate that the user has been informed that the
         operation cannot be performed.
 */
BOOLEAN
CoIsFileSelected(
    __in PCO_CONTEXT CoContext
    )
{
    DWORD Index;
    YORI_STRING Buttons[1];
    YORI_STRING Title;
    YORI_STRING Label;

    for (Index = 0; Index < CoContext->FilesFoundCount; Index++) {
        if (YoriWinListIsOptionSelected(CoContext->List, Index)) {
            return TRUE;
        }
    }

    YoriLibConstantString(&Buttons[0], _T("&Ok"));
    YoriLibConstantString(&Title, _T("Error"));
    YoriLibConstantString(&Label, _T("No files selected."));

    YoriDlgMessageBox(CoContext->WinMgr, &Title, &Label, 1, Buttons, 0, 0);
    return FALSE;
}

/**
 Remove any trailing newline characters from a string.

 @param String Pointer to the string to truncate.
 */
VOID
CoTrimTrailingNewlines(
    __inout PYORI_STRING String
    )
{
    DWORD Index;
    for (Index = String->LengthInChars; Index > 0; Index--) {
        if (String->StartOfString[Index - 1] != '\r' &&
            String->StartOfString[Index - 1] != '\n') {

            break;
        }
        String->LengthInChars--;
    }
}

/**
 Display a dialog to prompt the user for a directory to use as the target of
 a copy or move operation.

 @param CoContext Pointer to the program context including its window manager
        handle and current directory.

 @param TargetDirectory On successful completion, updated to point to a newly
        allocated string containing the target directory.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
CoGetTargetDirectory(
    __in PCO_CONTEXT CoContext,
    __out PYORI_STRING TargetDirectory
    )
{
    YORI_STRING Title;
    YORI_STRING Directory;
    YORI_STRING FullDir;
    YORI_STRING Label;
    YORI_STRING Buttons[2];
    YORI_STRING CurrentDirectory;
    DWORD FileAttr;

    YoriLibInitEmptyString(&Directory);
    YoriLibConstantString(&Title, _T("Enter Directory"));
    if (!YoriDlgDir(CoContext->WinMgr, &Title, 0, NULL, &Directory)) {
        return FALSE;
    }

    YoriLibInitEmptyString(&FullDir);
    if (!YoriLibUserStringToSingleFilePath(&Directory, TRUE, &FullDir)) {
        YoriLibFreeStringContents(&Directory);
        return FALSE;
    }

    //
    //  Translate the current directory into an escaped full path.  This is
    //  typically a non-escaped full path.
    //

    YoriLibInitEmptyString(&CurrentDirectory);
    if (!YoriLibGetFullPathNameReturnAllocation(&CoContext->CurrentDirectory, TRUE, &CurrentDirectory, NULL)) {
        YoriLibFreeStringContents(&FullDir);
        YoriLibFreeStringContents(&Directory);
        return FALSE;
    }

    if (YoriLibCompareStringInsensitive(&FullDir, &CurrentDirectory) == 0) {
        YoriLibConstantString(&Buttons[0], _T("&Ok"));
        YoriLibConstantString(&Title, _T("Error"));
        YoriLibConstantString(&Label, _T("Cannot move or copy files to current directory, which would overwrite source files"));
        YoriDlgMessageBox(CoContext->WinMgr, &Title, &Label, 1, Buttons, 0, 0);
        YoriLibFreeStringContents(&Directory);
        YoriLibFreeStringContents(&CurrentDirectory);
        YoriLibFreeStringContents(&FullDir);
        return FALSE;
    }

    YoriLibFreeStringContents(&CurrentDirectory);

    FileAttr = GetFileAttributes(FullDir.StartOfString);
    if ((FileAttr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        YoriLibConstantString(&Buttons[0], _T("&Ok"));
        YoriLibConstantString(&Title, _T("Error"));
        YoriLibConstantString(&Label, _T("Target is not a directory"));
        YoriDlgMessageBox(CoContext->WinMgr, &Title, &Label, 1, Buttons, 0, 0);
        YoriLibFreeStringContents(&Directory);
        YoriLibFreeStringContents(&FullDir);
        return FALSE;
    }

    if (FileAttr == (DWORD)-1) {
        DWORD ButtonId;

        YoriLibConstantString(&Buttons[0], _T("&Yes"));
        YoriLibConstantString(&Buttons[1], _T("&No"));
        YoriLibConstantString(&Title, _T("Create Directory"));

        YoriLibInitEmptyString(&Label);
        YoriLibYPrintf(&Label, _T("The directory \"%y\" does not exist.  Would you like to create it?"), &Directory);

        ButtonId = YoriDlgMessageBox(CoContext->WinMgr, &Title, &Label, 2, Buttons, 0, 1);
        YoriLibFreeStringContents(&Label);

        if (ButtonId != 1) {
            YoriLibFreeStringContents(&Directory);
            YoriLibFreeStringContents(&FullDir);
            return FALSE;
        }

        if (!CreateDirectory(FullDir.StartOfString, NULL)) {
            DWORD LastError;
            LPTSTR ErrText;
            LastError = GetLastError();
            ErrText = YoriLibGetWinErrorText(LastError);
            if (ErrText != NULL) {
                YoriLibYPrintf(&Label, _T("Could not create directory: %s"), ErrText);
                YoriLibFreeWinErrorText(ErrText);
                if (Label.LengthInChars > 0) {
                    YoriLibConstantString(&Buttons[0], _T("&Ok"));
                    YoriLibConstantString(&Title, _T("Error"));
                    YoriDlgMessageBox(CoContext->WinMgr, &Title, &Label, 1, Buttons, 0, 0);
                    YoriLibFreeStringContents(&Label);
                }
            }

            YoriLibFreeStringContents(&Directory);
            YoriLibFreeStringContents(&FullDir);
            return FALSE;
        }
    }

    YoriLibFreeStringContents(&Directory);
    memcpy(TargetDirectory, &FullDir, sizeof(YORI_STRING));
    return TRUE;
}


/**
 A callback invoked when the exit button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
CoExitButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    YoriWinCloseWindow(Parent, TRUE);
}

/**
 Global state about the program.  Used to pass information to button
 click handlers which don't have any other mechanism currently.
 */
CO_CONTEXT CoContext;

/**
 A callback invoked when the change directory button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
CoChdirButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    DWORD Index;
    BOOLEAN ListChanged = FALSE;
    YORI_STRING Buttons[1];
    YORI_STRING Title;
    YORI_STRING Label;
    YORI_STRING FullDir;
    DWORD LastError;
    DWORD FileAttr;
    LPTSTR ErrText;

    UNREFERENCED_PARAMETER(Ctrl);
    if (YoriWinListGetActiveOption(CoContext.List, &Index)) {
        if (CoContext.FileArray[Index]->IsDirectory) {

            YoriLibInitEmptyString(&FullDir);
            if (!YoriLibGetFullPathNameReturnAllocation(&CoContext.FileArray[Index]->FullFilePath, TRUE, &FullDir, NULL)) {
                LastError = GetLastError();
                ErrText = YoriLibGetWinErrorText(LastError);
                if (ErrText != NULL) {
                    YoriLibConstantString(&Buttons[0], _T("&Ok"));
                    YoriLibConstantString(&Title, _T("Error"));
                    YoriLibInitEmptyString(&Label);
                    YoriLibYPrintf(&Label, _T("Could not get full path for \"%y\": %s"), &CoContext.FileArray[Index]->FullFilePath, ErrText);
                    if (Label.LengthInChars > 0) {
                        CoTrimTrailingNewlines(&Label);
                        YoriDlgMessageBox(CoContext.WinMgr, &Title, &Label, 1, Buttons, 0, 0);
                        YoriLibFreeStringContents(&Label);
                    }
                    YoriLibFreeWinErrorText(ErrText);
                }
                return;
            }

            FileAttr = GetFileAttributes(FullDir.StartOfString);
            if (FileAttr == (DWORD)-1 ||
                ((FileAttr & FILE_ATTRIBUTE_DIRECTORY) == 0)) {

                YoriLibConstantString(&Buttons[0], _T("&Ok"));
                YoriLibConstantString(&Title, _T("Error"));
                YoriLibInitEmptyString(&Label);
                YoriLibYPrintf(&Label, _T("Could not change directory to \"%y\""), &FullDir);
                if (Label.LengthInChars > 0) {
                    CoTrimTrailingNewlines(&Label);
                    YoriDlgMessageBox(CoContext.WinMgr, &Title, &Label, 1, Buttons, 0, 0);
                    YoriLibFreeStringContents(&Label);
                }
                YoriLibFreeStringContents(&FullDir);
            } else {
                YoriLibFreeStringContents(&CoContext.CurrentDirectory);
                memcpy(&CoContext.CurrentDirectory, &FullDir, sizeof(YORI_STRING));
                ListChanged = TRUE;
            }
        }
    }
    if (ListChanged) {
        CoRepopulateList(&CoContext);
    }
}

/**
 A callback invoked when the delete button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
CoDeleteButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    DWORD Index;
    BOOLEAN ListChanged = FALSE;
    YORI_STRING Buttons[1];
    YORI_STRING Title;
    YORI_STRING Label;

    UNREFERENCED_PARAMETER(Ctrl);

    if (!CoIsFileSelected(&CoContext)) {
        return;
    }

    for (Index = 0; Index < CoContext.FilesFoundCount; Index++) {
        if (YoriWinListIsOptionSelected(CoContext.List, Index)) {
            if (!DeleteFile(CoContext.FileArray[Index]->FullFilePath.StartOfString)) {
                DWORD LastError;
                LPTSTR ErrText;
                LastError = GetLastError();
                ErrText = YoriLibGetWinErrorText(LastError);
                YoriLibConstantString(&Buttons[0], _T("&Ok"));
                YoriLibConstantString(&Title, _T("Error"));
                YoriLibInitEmptyString(&Label);
                YoriLibYPrintf(&Label, _T("Could not delete file \"%y\": %s"), &CoContext.FileArray[Index]->FullFilePath, ErrText);
                CoTrimTrailingNewlines(&Label);
                YoriLibFreeWinErrorText(ErrText);
                YoriDlgMessageBox(CoContext.WinMgr, &Title, &Label, 1, Buttons, 0, 0);
                YoriLibFreeStringContents(&Label);
                break;
            }
            ListChanged = TRUE;
        }
    }
    if (ListChanged) {
        CoRepopulateList(&CoContext);
    }
}

/**
 The width of each button, in characters.
 */
#define CO_BUTTON_WIDTH (16)

/**
 A callback invoked when the move button is clicked.

 @param ClickedCtrl Pointer to the button that was clicked.
 */
VOID
CoMoveButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE ClickedCtrl
    )
{
    YORI_STRING FullDir;
    YORI_STRING FullDest;
    DWORD Index;
    DWORD LastError;
    BOOLEAN ListChanged = FALSE;
    YORI_STRING Buttons[1];
    YORI_STRING Title;
    YORI_STRING Label;

    UNREFERENCED_PARAMETER(ClickedCtrl);

    if (!CoIsFileSelected(&CoContext)) {
        return;
    }

    if (!CoGetTargetDirectory(&CoContext, &FullDir)) {
        return;
    }

    if (!YoriLibAllocateString(&FullDest, FullDir.LengthAllocated + 256)) {
        YoriLibFreeStringContents(&FullDir);
        return;
    }

    ListChanged = FALSE;
    for (Index = 0; Index < CoContext.FilesFoundCount; Index++) {
        if (YoriWinListIsOptionSelected(CoContext.List, Index)) {

            FullDest.LengthInChars = YoriLibSPrintfS(FullDest.StartOfString, FullDest.LengthAllocated, _T("%y\\%y"), &FullDir, &CoContext.FileArray[Index]->DisplayName);

            LastError = YoriLibMoveFile(&CoContext.FileArray[Index]->FullFilePath, &FullDest, TRUE, FALSE);
            if (LastError != ERROR_SUCCESS) {
                LPTSTR ErrText;
                LastError = GetLastError();
                ErrText = YoriLibGetWinErrorText(LastError);
                if (ErrText != NULL) {
                    YoriLibConstantString(&Buttons[0], _T("&Ok"));
                    YoriLibConstantString(&Title, _T("Error"));
                    YoriLibInitEmptyString(&Label);
                    YoriLibYPrintf(&Label, _T("Could not move file from \"%y\" to \"%y\": %s"), &CoContext.FileArray[Index]->FullFilePath, &FullDest, ErrText);
                    if (Label.LengthInChars > 0) {
                        CoTrimTrailingNewlines(&Label);
                        YoriDlgMessageBox(CoContext.WinMgr, &Title, &Label, 1, Buttons, 0, 0);
                        YoriLibFreeStringContents(&Label);
                    }
                    YoriLibFreeWinErrorText(ErrText);
                }
                break;
            }
            ListChanged = TRUE;
        }
    }

    YoriLibFreeStringContents(&FullDest);
    YoriLibFreeStringContents(&FullDir);

    if (ListChanged) {
        CoRepopulateList(&CoContext);
    }
}

/**
 A callback invoked when the copy button is clicked.

 @param ClickedCtrl Pointer to the button that was clicked.
 */
VOID
CoCopyButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE ClickedCtrl
    )
{
    YORI_STRING FullDir;
    YORI_STRING FullDest;
    DWORD Index;
    DWORD LastError;
    BOOLEAN ListChanged = FALSE;
    YORI_STRING Buttons[1];
    YORI_STRING Title;
    YORI_STRING Label;

    UNREFERENCED_PARAMETER(ClickedCtrl);

    if (!CoIsFileSelected(&CoContext)) {
        return;
    }

    if (!CoGetTargetDirectory(&CoContext, &FullDir)) {
        return;
    }

    if (!YoriLibAllocateString(&FullDest, FullDir.LengthAllocated + 256)) {
        YoriLibFreeStringContents(&FullDir);
        return;
    }

    ListChanged = FALSE;
    for (Index = 0; Index < CoContext.FilesFoundCount; Index++) {
        if (YoriWinListIsOptionSelected(CoContext.List, Index)) {
            FullDest.LengthInChars = YoriLibSPrintfS(FullDest.StartOfString, FullDest.LengthAllocated, _T("%y\\%y"), &FullDir, &CoContext.FileArray[Index]->DisplayName);

            LastError = YoriLibCopyFile(&CoContext.FileArray[Index]->FullFilePath, &FullDest);
            if (LastError != ERROR_SUCCESS) {
                LPTSTR ErrText;
                LastError = GetLastError();
                ErrText = YoriLibGetWinErrorText(LastError);
                if (ErrText != NULL) {
                    YoriLibConstantString(&Buttons[0], _T("&Ok"));
                    YoriLibConstantString(&Title, _T("Error"));
                    YoriLibInitEmptyString(&Label);
                    YoriLibYPrintf(&Label, _T("Could not copy file from \"%y\" to \"%y\": %s"), &CoContext.FileArray[Index]->FullFilePath, &FullDest, ErrText);
                    if (Label.LengthInChars > 0) {
                        CoTrimTrailingNewlines(&Label);
                        YoriDlgMessageBox(CoContext.WinMgr, &Title, &Label, 1, Buttons, 0, 0);
                        YoriLibFreeStringContents(&Label);
                    }
                    YoriLibFreeWinErrorText(ErrText);
                }
                break;
            }
        }
    }

    YoriLibFreeStringContents(&FullDest);
    YoriLibFreeStringContents(&FullDir);

    if (ListChanged) {
        CoRepopulateList(&CoContext);
    }
}

/**
 Function invoked when the combo box selection changes and the sort order
 should change.

 @param ClickedCtrl Pointer to the control.
 */
VOID
CoSortSelected(
    __in PYORI_WIN_CTRL_HANDLE ClickedCtrl
    )
{
    DWORD ActiveIndex;

    if (YoriWinComboGetActiveOption(ClickedCtrl, &ActiveIndex)) {
        if (ActiveIndex < CoSortBeyondMaximum && ActiveIndex != (DWORD)CoContext.SortType) {
            CoContext.SortType = ActiveIndex;
            CoRepopulateList(&CoContext);
        }
    }
}

/**
 The minimum width in characters where co can hope to function.
 */
#define CO_MINIMUM_WIDTH (60)

/**
 The minimum height in characters where co can hope to function.
 */
#define CO_MINIMUM_HEIGHT (20)

/**
 Determine the size of the window and location of all controls for a specified
 window manager size.  This allows the same layout logic to be used for
 initial creation as well as window manager resize.

 @param WindowMgrSize The dimensions of the window manager.

 @param WindowSize On completion, updated to contain the size of the main
        window.

 @param ListRect On completion, updated to contain the rect describing the
        main list in window client coordinates.

 @param ExitButtonRect On completion, updated to contain the rect describing
        the exit button in window client coordinates.

 @param ChangeDirButtonRect On completion, updated to contain the rect
        describing the change dir button in window client coordinates.

 @param DeleteButtonRect On completion, updated to contain the rect
        describing the delete button in window client coordinates.

 @param MoveButtonRect On completion, updated to contain the rect describing
        the move button in window client coordinates.

 @param CopyButtonRect On completion, updated to contain the rect describing
        the copy button in window client coordinates.

 @param SortLabelRect On completion, updated to contain the rect describing
        the sort label in window client coordinates.

 @param SortComboRect On completion, updated to contain the rect describing
        the sort combo pull down in window client coordinates.
 */
VOID
CoGetControlRectsFromWindowManagerSize(
    __in COORD WindowMgrSize,
    __out PCOORD WindowSize,
    __out PSMALL_RECT ListRect,
    __out PSMALL_RECT ExitButtonRect,
    __out PSMALL_RECT ChangeDirButtonRect,
    __out PSMALL_RECT DeleteButtonRect,
    __out PSMALL_RECT MoveButtonRect,
    __out PSMALL_RECT CopyButtonRect,
    __out PSMALL_RECT SortLabelRect,
    __out PSMALL_RECT SortComboRect
    )
{
    COORD ClientSize;
    WORD ButtonHeight;
    WORD ButtonGap;

    WindowSize->X = (SHORT)(WindowMgrSize.X * 9 / 10);
    if (WindowSize->X < CO_MINIMUM_WIDTH) {
        WindowSize->X = CO_MINIMUM_WIDTH;
    }

    WindowSize->Y = (SHORT)(WindowMgrSize.Y * 4 / 5);
    if (WindowSize->Y < CO_MINIMUM_HEIGHT) {
        WindowSize->Y = CO_MINIMUM_HEIGHT;
    }

    ClientSize.X = (WORD)(WindowSize->X - 2);
    ClientSize.Y = (WORD)(WindowSize->Y - 2);

    ButtonHeight = 3;
    ButtonGap = 0;
    if (ClientSize.Y <= 20) {
        ButtonHeight = 1;
        ButtonGap = 1;
        if (ClientSize.Y <= 18) {
            ButtonGap = 0;
        }
    }

    ListRect->Left = 1;
    ListRect->Top = 1;
    ListRect->Right = (SHORT)(ClientSize.X - 3 - CO_BUTTON_WIDTH - 1 - 1);
    ListRect->Bottom = (SHORT)(ClientSize.Y - 1);

    ExitButtonRect->Top = (SHORT)(1);
    ExitButtonRect->Bottom = (SHORT)(ExitButtonRect->Top + ButtonHeight - 1);
    ExitButtonRect->Left = (SHORT)(ClientSize.X - 2 - CO_BUTTON_WIDTH - 1);
    ExitButtonRect->Right = (SHORT)(ExitButtonRect->Left + 1 + CO_BUTTON_WIDTH);

    ChangeDirButtonRect->Left = ExitButtonRect->Left;
    ChangeDirButtonRect->Right = ExitButtonRect->Right;
    ChangeDirButtonRect->Top = (SHORT)(ExitButtonRect->Bottom + 2 + ButtonGap);
    ChangeDirButtonRect->Bottom = (SHORT)(ChangeDirButtonRect->Top + ButtonHeight - 1);

    DeleteButtonRect->Left = ChangeDirButtonRect->Left;
    DeleteButtonRect->Right = ChangeDirButtonRect->Right;
    DeleteButtonRect->Top = (SHORT)(ChangeDirButtonRect->Bottom + 1 + ButtonGap);
    DeleteButtonRect->Bottom = (SHORT)(DeleteButtonRect->Top + ButtonHeight - 1);

    MoveButtonRect->Left = DeleteButtonRect->Left;
    MoveButtonRect->Right = DeleteButtonRect->Right;
    MoveButtonRect->Top = (SHORT)(DeleteButtonRect->Bottom + 1 + ButtonGap);
    MoveButtonRect->Bottom = (SHORT)(MoveButtonRect->Top + ButtonHeight - 1);

    CopyButtonRect->Left = MoveButtonRect->Left;
    CopyButtonRect->Right = MoveButtonRect->Right;
    CopyButtonRect->Top = (SHORT)(MoveButtonRect->Bottom + 1 + ButtonGap);
    CopyButtonRect->Bottom = (SHORT)(CopyButtonRect->Top + ButtonHeight - 1);

    SortLabelRect->Left = CopyButtonRect->Left;
    SortLabelRect->Right = CopyButtonRect->Right;
    SortLabelRect->Top = (SHORT)(CopyButtonRect->Bottom + 2 + ButtonGap);
    SortLabelRect->Bottom = SortLabelRect->Top;

    SortComboRect->Left = (SHORT)(SortLabelRect->Left + 1);
    SortComboRect->Right = SortLabelRect->Right;
    SortComboRect->Top = (SHORT)(SortLabelRect->Bottom + 1);
    SortComboRect->Bottom = SortComboRect->Top;
}

/**
 A callback that is invoked when the window manager is being resized.  This
 typically means the user resized the window.

 @param WindowHandle Handle to the main window.

 @param OldPosition The old dimensions of the window manager.

 @param NewPosition The new dimensions of the window manager.
 */
VOID
CoResizeWindowManager(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle,
    __in PSMALL_RECT OldPosition,
    __in PSMALL_RECT NewPosition
    )
{
    PYORI_WIN_CTRL_HANDLE WindowCtrl;
    SMALL_RECT Rect;
    COORD NewSize;
    COORD WindowSize;
    SMALL_RECT ListRect;
    SMALL_RECT ExitButtonRect;
    SMALL_RECT ChangeDirButtonRect;
    SMALL_RECT DeleteButtonRect;
    SMALL_RECT MoveButtonRect;
    SMALL_RECT CopyButtonRect;
    SMALL_RECT SortLabelRect;
    SMALL_RECT SortComboRect;
    PYORI_WIN_CTRL_HANDLE Ctrl;

    UNREFERENCED_PARAMETER(OldPosition);

    WindowCtrl = YoriWinGetCtrlFromWindow(WindowHandle);

    NewSize.X = (SHORT)(NewPosition->Right - NewPosition->Left + 1);
    NewSize.Y = (SHORT)(NewPosition->Bottom - NewPosition->Top + 1);

    if (NewSize.X < CO_MINIMUM_WIDTH || NewSize.Y < CO_MINIMUM_HEIGHT) {
        return;
    }

    CoGetControlRectsFromWindowManagerSize(NewSize,
                                           &WindowSize,
                                           &ListRect,
                                           &ExitButtonRect,
                                           &ChangeDirButtonRect,
                                           &DeleteButtonRect,
                                           &MoveButtonRect,
                                           &CopyButtonRect,
                                           &SortLabelRect,
                                           &SortComboRect);

    Rect.Left = (SHORT)((NewSize.X - WindowSize.X) / 2);
    Rect.Top = (SHORT)((NewSize.Y - WindowSize.Y) / 2);
    Rect.Right = (SHORT)(Rect.Left + WindowSize.X - 1);
    Rect.Bottom = (SHORT)(Rect.Top + WindowSize.Y - 1);

    //
    //  Resize the main window, including capturing its new background
    //

    if (!YoriWinWindowReposition(WindowHandle, &Rect)) {
        return;
    }

    Ctrl = YoriWinFindControlById(WindowHandle, CoCtrlList);
    ASSERT(Ctrl != NULL);
    YoriWinListReposition(Ctrl, &ListRect);

    Ctrl = YoriWinFindControlById(WindowHandle, CoCtrlExit);
    ASSERT(Ctrl != NULL);
    YoriWinButtonReposition(Ctrl, &ExitButtonRect);

    Ctrl = YoriWinFindControlById(WindowHandle, CoCtrlChangeDir);
    ASSERT(Ctrl != NULL);
    YoriWinButtonReposition(Ctrl, &ChangeDirButtonRect);

    Ctrl = YoriWinFindControlById(WindowHandle, CoCtrlDelete);
    ASSERT(Ctrl != NULL);
    YoriWinButtonReposition(Ctrl, &DeleteButtonRect);

    Ctrl = YoriWinFindControlById(WindowHandle, CoCtrlMove);
    ASSERT(Ctrl != NULL);
    YoriWinButtonReposition(Ctrl, &MoveButtonRect);

    Ctrl = YoriWinFindControlById(WindowHandle, CoCtrlCopy);
    ASSERT(Ctrl != NULL);
    YoriWinButtonReposition(Ctrl, &CopyButtonRect);

    Ctrl = YoriWinFindControlById(WindowHandle, CoCtrlSortLabel);
    ASSERT(Ctrl != NULL);
    YoriWinLabelReposition(Ctrl, &SortLabelRect);

    Ctrl = YoriWinFindControlById(WindowHandle, CoCtrlSortCombo);
    ASSERT(Ctrl != NULL);
    YoriWinComboReposition(Ctrl, &SortComboRect);
}


/**
 Display a popup window containing a list of files and file operations.

 @return TRUE to indicate that the user successfully selected an option, FALSE
         to indicate the menu could not be displayed or the user cancelled the
         operation.
 */
__success(return)
BOOL
CoCreateSynchronousMenu(VOID)
{
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgr;
    PYORI_WIN_CTRL_HANDLE List;
    PYORI_WIN_WINDOW_HANDLE Parent;
    SMALL_RECT ListRect;
    SMALL_RECT ExitButtonRect;
    SMALL_RECT ChangeDirButtonRect;
    SMALL_RECT DeleteButtonRect;
    SMALL_RECT MoveButtonRect;
    SMALL_RECT CopyButtonRect;
    SMALL_RECT SortLabelRect;
    SMALL_RECT SortComboRect;
    COORD WindowSize;
    YORI_STRING Title;
    SMALL_RECT ButtonArea;
    YORI_STRING Caption;
    WORD ButtonWidth;
    PYORI_WIN_CTRL_HANDLE Ctrl;
    DWORD_PTR Result;
    YORI_STRING SortStrings[CoSortBeyondMaximum];

    if (!YoriWinOpenWindowManager(FALSE, &WinMgr)) {
        return FALSE;
    }

    if (!YoriWinGetWinMgrDimensions(WinMgr, &WindowSize)) {
        WindowSize.X = 80;
        WindowSize.Y = 24;
    }

    CoGetControlRectsFromWindowManagerSize(WindowSize,
                                           &WindowSize,
                                           &ListRect,
                                           &ExitButtonRect,
                                           &ChangeDirButtonRect,
                                           &DeleteButtonRect,
                                           &MoveButtonRect,
                                           &CopyButtonRect,
                                           &SortLabelRect,
                                           &SortComboRect);

    YoriLibConstantString(&Title, _T("Co"));

    if (!YoriWinCreateWindow(WinMgr, WindowSize.X, WindowSize.Y, WindowSize.X, WindowSize.Y, YORI_WIN_WINDOW_STYLE_BORDER_SINGLE | YORI_WIN_WINDOW_STYLE_SHADOW_SOLID, &Title, &Parent)) {
        YoriWinCloseWindowManager(WinMgr);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("co: Could not display window: terminal too small?\n"));
        return FALSE;
    }

    YoriWinGetClientSize(Parent, &WindowSize);

    List = YoriWinListCreate(Parent, &ListRect, YORI_WIN_LIST_STYLE_VSCROLLBAR | YORI_WIN_LIST_STYLE_MULTISELECT);
    if (List == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriWinSetControlId(List, CoCtrlList);
    ButtonWidth = (WORD)(CO_BUTTON_WIDTH);

    ButtonArea.Top = (SHORT)(1);
    ButtonArea.Bottom = (SHORT)(3);

    YoriLibConstantString(&Caption, _T("E&xit"));

    ButtonArea.Left = (SHORT)(WindowSize.X - 2 - CO_BUTTON_WIDTH - 1);
    ButtonArea.Right = (WORD)(ButtonArea.Left + 1 + CO_BUTTON_WIDTH);

    Ctrl = YoriWinButtonCreate(Parent, &ExitButtonRect, &Caption, YORI_WIN_BUTTON_STYLE_CANCEL, CoExitButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }
    YoriWinSetControlId(Ctrl, CoCtrlExit);

    ButtonArea.Top += 4;
    ButtonArea.Bottom += 4;

    YoriLibConstantString(&Caption, _T("C&hange Dir"));
    Ctrl = YoriWinButtonCreate(Parent, &ChangeDirButtonRect, &Caption, YORI_WIN_BUTTON_STYLE_DEFAULT, CoChdirButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }
    YoriWinSetControlId(Ctrl, CoCtrlChangeDir);

    ButtonArea.Top += 3;
    ButtonArea.Bottom += 3;

    YoriLibConstantString(&Caption, _T("&Delete"));
    Ctrl = YoriWinButtonCreate(Parent, &DeleteButtonRect, &Caption, 0, CoDeleteButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }
    YoriWinSetControlId(Ctrl, CoCtrlDelete);

    ButtonArea.Top += 3;
    ButtonArea.Bottom += 3;

    YoriLibConstantString(&Caption, _T("&Move"));
    Ctrl = YoriWinButtonCreate(Parent, &MoveButtonRect, &Caption, 0, CoMoveButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }
    YoriWinSetControlId(Ctrl, CoCtrlMove);

    ButtonArea.Top += 3;
    ButtonArea.Bottom += 3;

    YoriLibConstantString(&Caption, _T("&Copy"));
    Ctrl = YoriWinButtonCreate(Parent, &CopyButtonRect, &Caption, 0, CoCopyButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }
    YoriWinSetControlId(Ctrl, CoCtrlCopy);

    ButtonArea.Top += 4;
    ButtonArea.Bottom = ButtonArea.Top;

    YoriLibConstantString(&Caption, _T("&Sort:"));
    Ctrl = YoriWinLabelCreate(Parent, &SortLabelRect, &Caption, 0);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }
    YoriWinSetControlId(Ctrl, CoCtrlSortLabel);

    ButtonArea.Top += 1;
    ButtonArea.Bottom = ButtonArea.Top;
    ButtonArea.Left += 1;

    YoriLibConstantString(&Caption, _T(""));
    Ctrl = YoriWinComboCreate(Parent, &SortComboRect, CoSortBeyondMaximum, &Caption, 0, CoSortSelected);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }
    YoriWinSetControlId(Ctrl, CoCtrlSortCombo);

    YoriLibConstantString(&SortStrings[CoSortByName], _T("Sort by Name"));
    YoriLibConstantString(&SortStrings[CoSortBySize], _T("Sort by Size"));
    YoriLibConstantString(&SortStrings[CoSortByDate], _T("Sort by Date"));

    YoriWinComboAddItems(Ctrl, SortStrings, CoSortBeyondMaximum);
    YoriWinComboSetActiveOption(Ctrl, CoContext.SortType);

    YoriLibInitializeListHead(&CoContext.FilesFound);
    CoContext.FilesFoundCount = 0;
    CoContext.FileArray = NULL;
    CoContext.List = List;
    CoContext.WinMgr = WinMgr;
    if (!YoriLibGetCurrentDirectory(&CoContext.CurrentDirectory)) {
        CoFreeContext(&CoContext);
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        CoContext.WinMgr = NULL;
        return FALSE;
    }

    if (!CoPopulateList(&CoContext)) {
        CoFreeContext(&CoContext);
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        CoContext.WinMgr = NULL;
        return FALSE;
    }

    YoriWinSetWindowManagerResizeNotifyCallback(Parent, CoResizeWindowManager);

    Result = FALSE;
    if (!YoriWinProcessInputForWindow(Parent, &Result)) {
        Result = FALSE;
    }

    CoFreeContext(&CoContext);

    YoriWinDestroyWindow(Parent);
    YoriWinCloseWindowManager(WinMgr);
    CoContext.WinMgr = NULL;
    return (BOOL)Result;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the co builtin command.
 */
#define ENTRYPOINT YoriCmd_YCO
#else
/**
 The main entrypoint for the co standalone application.
 */
#define ENTRYPOINT ymain
#endif


/**
 Display yori shell simple file manager.

 @param ArgC The number of arguments.

 @param ArgV The argument array.

 @return ExitCode, zero for success, nonzero for failure.
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
    YORI_STRING Arg;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                CoHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2019-2021"));
                return EXIT_SUCCESS;
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

    YoriLibLoadAdvApi32Functions();

    if (!CoCreateSynchronousMenu()) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
