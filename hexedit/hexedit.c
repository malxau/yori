/**
 * @file hexedit/hexedit.c
 *
 * Yori shell hex editor
 *
 * Copyright (c) 2020-2023 Malcolm J. Smith
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
 Help text to display to the user.
 */
const
CHAR strHexEditHelpText[] =
        "\n"
        "Displays hexeditor.\n"
        "\n"
        "HEXEDIT [-license] [-a] [-r] [filename]\n"
        "\n"
        "   -a             Use ASCII characters for drawing\n"
        "   -r             Open file as read only\n";

/**
 The copyright year string to display with license text.
 */
const
TCHAR strHexEditCopyrightYear[] = _T("2020-2023");

/**
 Display usage text to the user.
 */
BOOL
HexEditHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("HexEdit %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHexEditHelpText);
    return TRUE;
}

/**
 A context that records files found and being operated on in the current
 window.
 */
typedef struct _HEXEDIT_CONTEXT {

    /**
     Pointer to the multiline hexedit control.
     */
    PYORI_WIN_CTRL_HANDLE HexEdit;

    /**
     Pointer to the menu bar control.
     */
    PYORI_WIN_CTRL_HANDLE MenuBar;

    /**
     Pointer to the status bar control.
     */
    PYORI_WIN_CTRL_HANDLE StatusBar;

    /**
     Pointer to the window manager.
     */
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgr;

    /**
     The string for the file to open, and the name to use when saving.
     */
    YORI_STRING OpenFileName;

    /**
     The offset within the file corresponding to the currently edited data.
     */
    DWORDLONG DataOffset;

    /**
     The length of the range of the currently edited data.
     */
    DWORDLONG DataLength;

    /**
     The string that was most recently searched for.
     */
    YORI_STRING SearchString;

    /**
     The index of the hexedit menu.  This is used to enable and disable menu
     items based on the state of the control and clipboard.
     */
    DWORD EditMenuIndex;

    /**
     The index of the cut menu item.
     */
    DWORD HexEditCutMenuIndex;

    /**
     The index of the copy menu item.
     */
    DWORD HexEditCopyMenuIndex;

    /**
     The index of the paste menu item.
     */
    DWORD HexEditPasteMenuIndex;

    /**
     The index of the clear menu item.
     */
    DWORD HexEditClearMenuIndex;

    /**
     TRUE if the search should be case sensitive.  FALSE if it should be case
     insensitive.
     */
    BOOLEAN SearchMatchCase;

    /**
     TRUE to use only 7 bit ASCII characters for visual display.
     */
    BOOLEAN UseAsciiDrawing;

    /**
     TRUE if the current file is opened read only, FALSE if it is opened for
     hexediting.
     */
    BOOLEAN ReadOnly;

} HEXEDIT_CONTEXT, *PHEXEDIT_CONTEXT;

/**
 Global context for the hexedit application.
 */
HEXEDIT_CONTEXT GlobalHexEditContext;

/**
 Free all found files in the list.

 @param HexEditContext Pointer to the hexedit context.
 */
VOID
HexEditFreeHexEditContext(
    __in PHEXEDIT_CONTEXT HexEditContext
    )
{
    YoriLibFreeStringContents(&HexEditContext->OpenFileName);
    YoriLibFreeStringContents(&HexEditContext->SearchString);
}

/**
 Set the caption on the hexedit control to match the file name component of the
 currently opened file.
 */
VOID
HexEditUpdateOpenedFileCaption(
    __in PHEXEDIT_CONTEXT HexEditContext
    )
{
    YORI_STRING NewCaption;
    LPTSTR FinalSlash;

    YoriLibInitEmptyString(&NewCaption);

    FinalSlash = YoriLibFindRightMostCharacter(&HexEditContext->OpenFileName, '\\');
    if (FinalSlash != NULL) {
        NewCaption.StartOfString = FinalSlash + 1;
        NewCaption.LengthInChars = (DWORD)(HexEditContext->OpenFileName.LengthInChars - (FinalSlash - HexEditContext->OpenFileName.StartOfString + 1));
    } else {
        NewCaption.StartOfString = HexEditContext->OpenFileName.StartOfString;
        NewCaption.LengthInChars = HexEditContext->OpenFileName.LengthInChars;
    }

    YoriWinHexEditSetCaption(HexEditContext->HexEdit, &NewCaption);
}

/**
 Load the contents of the specified file into the hexedit window.

 @param HexEditContext Pointer to the hexedit context.

 @param FileName Pointer to the name of the file to open.

 @param DataOffset Specifies the offset within the file to load the data.

 @param DataLength Specifies the number of bytes of data to load.  If zero,
        the entire file or device contents are loaded.

 @return Win32 error code, including ERROR_SUCCESS to indicate success.
 */
DWORD
HexEditLoadFile(
    __in PHEXEDIT_CONTEXT HexEditContext,
    __in PYORI_STRING FileName,
    __in DWORDLONG DataOffset,
    __in DWORDLONG DataLength
    )
{
    HANDLE hFile;
    LARGE_INTEGER FileSize;
    LARGE_INTEGER FileOffset;
    PUCHAR Buffer;
    DWORD BytesRead;
    DWORD Err;

    if (FileName->StartOfString == NULL) {
        return ERROR_INVALID_NAME;
    }

    ASSERT(YoriLibIsStringNullTerminated(FileName));

    hFile = CreateFile(FileName->StartOfString, FILE_READ_DATA | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }

    // 
    //  MSFIX This doesn't make any sense for a device.  We could detect the
    //  error and try IOCTL_DISK_GET_LENGTH_INFO, although for a device we
    //  probably should receive the size to access.
    //

    if (DataLength == 0) {
        Err = YoriLibGetFileOrDeviceSize(hFile, &FileSize.QuadPart);
        if (Err != ERROR_SUCCESS) {
            CloseHandle(hFile);
            return Err;
        }
    } else {
        FileSize.QuadPart = DataLength;
    }

    if (FileSize.HighPart != 0) {
        CloseHandle(hFile);
        return ERROR_READ_FAULT;
    }

    FileOffset.QuadPart = DataOffset;
    if (FileOffset.QuadPart != 0) {
        if (!SetFilePointer(hFile, FileOffset.LowPart, &FileOffset.HighPart, FILE_BEGIN)) {
            Err = GetLastError();
            CloseHandle(hFile);
            return Err;
        }
    }

    Buffer = YoriLibReferencedMalloc(FileSize.LowPart);
    if (Buffer == NULL) {
        CloseHandle(hFile);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (!ReadFile(hFile, Buffer, FileSize.LowPart, &BytesRead, NULL)) {
        Err = GetLastError();
        YoriLibDereference(Buffer);
        CloseHandle(hFile);
        return Err;
    }

    //
    //  If the call requested us to automatically detect the length, tolerate
    //  it being a little smaller than we expect.  This happens when the
    //  partition layer passes IOCTLs to the disk, sigh.
    //

    if (BytesRead > FileSize.LowPart ||
        (DataLength != 0 && BytesRead != FileSize.LowPart)) {

        YoriLibDereference(Buffer);
        CloseHandle(hFile);
        return ERROR_INVALID_DATA;
    }

    CloseHandle(hFile);

    YoriWinHexEditClear(HexEditContext->HexEdit);

    if (!YoriWinHexEditSetDataNoCopy(HexEditContext->HexEdit, Buffer, FileSize.LowPart, BytesRead)) {
        YoriLibDereference(Buffer);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    YoriLibDereference(Buffer);

    HexEditContext->DataOffset = DataOffset;
    HexEditContext->DataLength = DataLength;

    return ERROR_SUCCESS;
}

/**
 Save the contents of the opened window into a file.

 @param HexEditContext Pointer to the hexedit context.

 @param FileName Pointer to the name of the file to save.

 @param DataOffset Specifies the offset within the file to store the data.

 @param DataLength Specifies the number of bytes of data to store.  If this
        differs from the current buffer, the save operation is failed.  If
        this is zero, the number of bytes is undefined, and any value in the
        hex edit control buffer can be used.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
HexEditSaveFile(
    __in PHEXEDIT_CONTEXT HexEditContext,
    __in PYORI_STRING FileName,
    __in DWORDLONG DataOffset,
    __in DWORDLONG DataLength
    )
{
    DWORD Index;
    DWORD Attributes;
    YORI_STRING ParentDirectory;
    YORI_STRING Prefix;
    YORI_STRING TempFileName;
    HANDLE TempHandle;
    BOOLEAN ReplaceSucceeded;
    PUCHAR Buffer;
    DWORDLONG BufferLength;
    DWORD BufferWritten;

    if (FileName->StartOfString == NULL) {
        return FALSE;
    }

    ASSERT(YoriLibIsStringNullTerminated(FileName));

    //
    //  Find the parent directory of the user specified file so a temporary
    //  file can be created in the same directory.  This is done to increase
    //  the chance that the file is written to the same device as the final
    //  location and to test that the user can write to the location.
    //

    YoriLibInitEmptyString(&ParentDirectory);
    for (Index = FileName->LengthInChars; Index > 0; Index--) {
        if (YoriLibIsSep(FileName->StartOfString[Index - 1])) {
            ParentDirectory.StartOfString = FileName->StartOfString;
            ParentDirectory.LengthInChars = Index - 1;
            break;
        }
    }

    if (Index == 0) {
        YoriLibConstantString(&ParentDirectory, _T("."));
    }

    YoriLibConstantString(&Prefix, _T("YEDT"));

    if (!YoriLibGetTempFileName(&ParentDirectory, &Prefix, &TempHandle, &TempFileName)) {
        return FALSE;
    }

    YoriWinHexEditGetDataNoCopy(HexEditContext->HexEdit, &Buffer, &BufferLength);

    if (DataLength != 0 &&
        DataLength != BufferLength) {

        // MSFIX Display warning
    }

    if (DataOffset != 0) {
        LARGE_INTEGER FileOffset;
        FileOffset.QuadPart = HexEditContext->DataOffset;
        if (!SetFilePointer(TempHandle, FileOffset.LowPart, &FileOffset.HighPart, FILE_BEGIN)) {
            CloseHandle(TempHandle);
            DeleteFile(TempFileName.StartOfString);
            YoriLibFreeStringContents(&TempFileName);
            return FALSE;
        }
    }

    if (Buffer != NULL) {

        //
        //  MSFIX This is truncating to 4Gb but is probably limited much lower
        //

        if (!WriteFile(TempHandle, Buffer, (DWORD)BufferLength, &BufferWritten, NULL)) {
            CloseHandle(TempHandle);
            DeleteFile(TempFileName.StartOfString);
            YoriLibFreeStringContents(&TempFileName);
            return FALSE;
        }

        YoriLibDereference(Buffer);
    }

    //
    //  Flush the temporary file to ensure it's durable, and rename it over
    //  the top of the chosen file, replacing if necessary.  This ensures
    //  that the old contents are not deleted until the new contents are
    //  successfully written.
    //

    if (!FlushFileBuffers(TempHandle)) {
        CloseHandle(TempHandle);
        DeleteFile(TempFileName.StartOfString);
        YoriLibFreeStringContents(&TempFileName);
        return FALSE;
    }

    CloseHandle(TempHandle);

    //
    //  If the file exists and ReplaceFile is present, replace it. Without
    //  ReplaceFile or if the file doesn't exist, rename the temporary file
    //  into place.  If ReplaceFile fails for whatever reason, fall back to
    //  rename, which implicitly prioritizes succeeding the save to preserving
    //  whatever file metadata ReplaceFile is aiming to retain.
    //

    ReplaceSucceeded = FALSE;
    Attributes = GetFileAttributes(FileName->StartOfString);
    if (Attributes != (DWORD)-1 &&
        DllKernel32.pReplaceFileW != NULL) {

        if (!DllKernel32.pReplaceFileW(FileName->StartOfString, TempFileName.StartOfString, NULL, 0, NULL, NULL)) {
            DeleteFile(TempFileName.StartOfString);
            YoriLibFreeStringContents(&TempFileName);
            return FALSE;
        }
        ReplaceSucceeded = TRUE;
    }

    if (!ReplaceSucceeded) {
        if (!MoveFileEx(TempFileName.StartOfString, FileName->StartOfString, MOVEFILE_REPLACE_EXISTING)) {
            DeleteFile(TempFileName.StartOfString);
            YoriLibFreeStringContents(&TempFileName);
            return FALSE;
        }
    }

    YoriLibFreeStringContents(&TempFileName);
    return TRUE;
}

VOID
HexEditSaveButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    );

VOID
HexEditSaveAsButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    );

/**
 If the file has been modified, prompt the user to save it, and save it if
 requested.

 @param Ctrl Pointer to the menu control indicating the action that triggered
        this prompt.

 @param HexEditContext Pointer to the hexedit context.

 @return TRUE to indicate that the requested action should proceed, FALSE to
         indicate the user has cancelled the request.
 */
BOOLEAN
HexEditPromptForSaveIfModified(
    __in PYORI_WIN_CTRL_HANDLE Ctrl,
    __in PHEXEDIT_CONTEXT HexEditContext
    )
{
    if (YoriWinHexEditGetModifyState(HexEditContext->HexEdit)) {
        PYORI_WIN_CTRL_HANDLE Parent;
        YORI_STRING Title;
        YORI_STRING Text;
        YORI_STRING ButtonText[3];
        DWORD ButtonId;

        Parent = YoriWinGetControlParent(HexEditContext->HexEdit);

        YoriLibConstantString(&Title, _T("Save changes"));
        YoriLibConstantString(&Text, _T("The file has been modified.  Save changes?"));
        YoriLibConstantString(&ButtonText[0], _T("&Yes"));
        YoriLibConstantString(&ButtonText[1], _T("&No"));
        YoriLibConstantString(&ButtonText[2], _T("&Cancel"));

        ButtonId = YoriDlgMessageBox(YoriWinGetWindowManagerHandle(Parent),
                                     &Title,
                                     &Text,
                                     3,
                                     ButtonText,
                                     0,
                                     2);

        //
        //  If the dialog failed or the cancel button was pressed, don't
        //  exit.
        //

        if (ButtonId == 0 || ButtonId == 3) {
            return FALSE;
        }

        //
        //  If the save button was clicked, invoke save or save as depending
        //  on whether a file name is present.
        //

        if (ButtonId == 1) {
            if (HexEditContext->OpenFileName.LengthInChars > 0) {
                HexEditSaveButtonClicked(Ctrl);
            } else {
                HexEditSaveAsButtonClicked(Ctrl);
            }

            //
            //  If the buffer is still modified, that implies the save didn't
            //  happen, so cancel.
            //

            if (YoriWinHexEditGetModifyState(HexEditContext->HexEdit)) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

/**
 A callback invoked when the new menu item is invoked.

 @param Ctrl Pointer to the menu bar control.
 */
VOID
HexEditNewButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PHEXEDIT_CONTEXT HexEditContext;
    PYORI_WIN_CTRL_HANDLE Parent;

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    if (!HexEditPromptForSaveIfModified(Ctrl, HexEditContext)) {
        return;
    }

    YoriWinHexEditClear(HexEditContext->HexEdit);
    YoriLibFreeStringContents(&HexEditContext->OpenFileName);
    HexEditUpdateOpenedFileCaption(HexEditContext);
    YoriWinHexEditSetModifyState(HexEditContext->HexEdit, FALSE);
}

/**
 Display an open dialog.  The open may be for files or devices.

 @param Parent Handle to the main window.

 @param HexEditContext Pointer to the hexedit context.

 @param OpenDevice If TRUE, the open device dialog should be displayed.
        If FALSE, the open file dialog should be displayed.
 */
VOID
HexEditOpenDialog(
    __in PYORI_WIN_CTRL_HANDLE Parent,
    __in PHEXEDIT_CONTEXT HexEditContext,
    __in BOOLEAN OpenDevice
    )
{
    YORI_STRING Title;
    YORI_STRING Text;
    YORI_STRING FullName;
    YORI_DLG_FILE_CUSTOM_VALUE ReadOnlyValues[2];
    YORI_DLG_FILE_CUSTOM_OPTION CustomOptionArray[1];
    DWORD Err;
    DWORDLONG DeviceOffset;
    DWORDLONG DeviceLength;

    YoriLibConstantString(&ReadOnlyValues[0].ValueText, _T("Open for editing"));
    YoriLibConstantString(&ReadOnlyValues[1].ValueText, _T("Open read only"));

    YoriLibConstantString(&CustomOptionArray[0].Description, _T("&Read Only:"));
    CustomOptionArray[0].ValueCount = 2;
    CustomOptionArray[0].Values = ReadOnlyValues;
    if (HexEditContext->ReadOnly) {
        CustomOptionArray[0].SelectedValue = 1;
    } else {
        CustomOptionArray[0].SelectedValue = 0;
    }

    YoriLibConstantString(&Title, _T("Open"));
    YoriLibInitEmptyString(&Text);

    DeviceOffset = 0;
    DeviceLength = 0;

    if (OpenDevice) {
        YoriDlgDevice(YoriWinGetWindowManagerHandle(Parent),
                      &Title,
                      sizeof(CustomOptionArray)/sizeof(CustomOptionArray[0]),
                      CustomOptionArray,
                      &Text,
                      &DeviceOffset,
                      &DeviceLength);

    } else {
        YoriDlgFile(YoriWinGetWindowManagerHandle(Parent),
                    &Title,
                    sizeof(CustomOptionArray)/sizeof(CustomOptionArray[0]),
                    CustomOptionArray,
                    &Text);
    }

    if (Text.LengthInChars == 0) {
        YoriLibFreeStringContents(&Text);
        return;
    }
    YoriLibInitEmptyString(&FullName);

    if (!YoriLibUserStringToSingleFilePath(&Text, TRUE, &FullName)) {
        YoriLibFreeStringContents(&Text);
        return;
    }

    YoriLibFreeStringContents(&Text);
    Err = HexEditLoadFile(HexEditContext, &FullName, DeviceOffset, DeviceLength);
    if (Err != ERROR_SUCCESS) {
        YORI_STRING DialogText;
        YORI_STRING ButtonText;
        LPTSTR ErrText;

        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibInitEmptyString(&DialogText);
        YoriLibYPrintf(&DialogText, _T("Could not open file: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);

        YoriLibConstantString(&ButtonText, _T("Ok"));

        YoriDlgMessageBox(YoriWinGetWindowManagerHandle(Parent),
                          &Title,
                          &DialogText,
                          1,
                          &ButtonText,
                          0,
                          0);

        YoriLibFreeStringContents(&DialogText);
        YoriLibFreeStringContents(&FullName);
        return;
    }

    YoriLibFreeStringContents(&HexEditContext->OpenFileName);
    memcpy(&HexEditContext->OpenFileName, &FullName, sizeof(YORI_STRING));
    HexEditUpdateOpenedFileCaption(HexEditContext);
    YoriWinHexEditSetModifyState(HexEditContext->HexEdit, FALSE);

    if (CustomOptionArray[0].SelectedValue != 0) {
        HexEditContext->ReadOnly = TRUE;
    } else {
        HexEditContext->ReadOnly = FALSE;
    }

    YoriWinHexEditSetReadOnly(HexEditContext->HexEdit, HexEditContext->ReadOnly);
}

/**
 A callback invoked when the open menu item is invoked.

 @param Ctrl Pointer to the menu bar control.
 */
VOID
HexEditOpenButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PHEXEDIT_CONTEXT HexEditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    HexEditOpenDialog(Parent, HexEditContext, FALSE);
}

/**
 A callback invoked when the open menu item is invoked.

 @param Ctrl Pointer to the menu bar control.
 */
VOID
HexEditOpenDeviceButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PHEXEDIT_CONTEXT HexEditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    HexEditOpenDialog(Parent, HexEditContext, TRUE);
}

VOID
HexEditSaveAsButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    );

/**
 A callback invoked when the save menu item is invoked.

 @param Ctrl Pointer to the menu bar control.
 */
VOID
HexEditSaveButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    YORI_STRING Title;
    YORI_STRING Text;
    YORI_STRING ButtonText;
    PHEXEDIT_CONTEXT HexEditContext;

    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    if (HexEditContext->OpenFileName.StartOfString == NULL) {
        HexEditSaveAsButtonClicked(Ctrl);
        return;
    }

    YoriLibConstantString(&Title, _T("Save"));
    YoriLibConstantString(&ButtonText, _T("Ok"));

    if (!HexEditSaveFile(HexEditContext, &HexEditContext->OpenFileName, HexEditContext->DataOffset, HexEditContext->DataLength)) {
        YoriLibConstantString(&Text, _T("Could not open file for writing"));

        YoriDlgMessageBox(YoriWinGetWindowManagerHandle(Parent),
                          &Title,
                          &Text,
                          1,
                          &ButtonText,
                          0,
                          0);

        return;
    }
    YoriWinHexEditSetModifyState(HexEditContext->HexEdit, FALSE);
}

/**
 A callback invoked when the save as menu item is invoked.

 @param Ctrl Pointer to the menu bar control.
 */
VOID
HexEditSaveAsButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    YORI_STRING Title;
    YORI_STRING Text;
    YORI_STRING FullName;
    PHEXEDIT_CONTEXT HexEditContext;

    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    YoriLibConstantString(&Title, _T("Save As"));
    YoriLibInitEmptyString(&Text);

    YoriDlgFile(YoriWinGetWindowManagerHandle(Parent),
                &Title,
                0,
                NULL,
                &Text);

    if (Text.LengthInChars == 0) {
        YoriLibFreeStringContents(&Text);
        return;
    }
    YoriLibInitEmptyString(&FullName);

    if (!YoriLibUserStringToSingleFilePath(&Text, TRUE, &FullName)) {
        YoriLibFreeStringContents(&Text);
        return;
    }

    YoriLibFreeStringContents(&Text);
    if (!HexEditSaveFile(HexEditContext, &FullName, 0, 0)) {
        YORI_STRING ButtonText;

        YoriLibFreeStringContents(&FullName);
        YoriLibConstantString(&ButtonText, _T("Ok"));
        YoriLibConstantString(&Text, _T("Could not open file for writing"));

        YoriDlgMessageBox(YoriWinGetWindowManagerHandle(Parent),
                          &Title,
                          &Text,
                          1,
                          &ButtonText,
                          0,
                          0);
        return;
    }

    YoriLibFreeStringContents(&HexEditContext->OpenFileName);
    memcpy(&HexEditContext->OpenFileName, &FullName, sizeof(YORI_STRING));
    HexEditUpdateOpenedFileCaption(HexEditContext);
    YoriWinHexEditSetModifyState(HexEditContext->HexEdit, FALSE);
    HexEditContext->DataOffset = 0;
    HexEditContext->DataLength = 0;
}

/**
 A callback invoked when the exit button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
HexEditExitButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PHEXEDIT_CONTEXT HexEditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    if (!HexEditPromptForSaveIfModified(Ctrl, HexEditContext)) {
        return;
    }

    YoriWinCloseWindow(Parent, TRUE);
}

/**
 A callback invoked when the go to menu item is invoked.

 @param Ctrl Pointer to the menu bar control.
 */
VOID
HexEditGoToButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    YORI_STRING Title;
    YORI_STRING Text;
    PYORI_WIN_CTRL_HANDLE Parent;
    PHEXEDIT_CONTEXT HexEditContext;
    LONGLONG NewOffset;
    DWORD CharsConsumed;

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    YoriLibConstantString(&Title, _T("Go to"));
    YoriLibInitEmptyString(&Text);

    YoriDlgInput(YoriWinGetWindowManagerHandle(Parent),
                 &Title,
                 TRUE,
                 &Text);

    if (Text.LengthInChars == 0) {
        YoriLibFreeStringContents(&Text);
        return;
    }

    if (YoriLibStringToNumber(&Text, FALSE, &NewOffset, &CharsConsumed) &&
        CharsConsumed > 0) {

        if (NewOffset < 0) {
            NewOffset = 0;
        }

        YoriWinHexEditSetCursorLocation(HexEditContext->HexEdit, FALSE, NewOffset, 0);
    }

    YoriLibFreeStringContents(&Text);
}

/**
 A callback invoked when the view bytes button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
HexEditViewBytesButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PHEXEDIT_CONTEXT HexEditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    YoriWinHexEditSetBytesPerWord(HexEditContext->HexEdit, 1);
}

/**
 A callback invoked when the view words button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
HexEditViewWordsButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PHEXEDIT_CONTEXT HexEditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    YoriWinHexEditSetBytesPerWord(HexEditContext->HexEdit, 2);
}

/**
 A callback invoked when the view dwords button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
HexEditViewDwordsButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PHEXEDIT_CONTEXT HexEditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    YoriWinHexEditSetBytesPerWord(HexEditContext->HexEdit, 4);
}

/**
 A callback invoked when the view qwords button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
HexEditViewQwordsButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PHEXEDIT_CONTEXT HexEditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    YoriWinHexEditSetBytesPerWord(HexEditContext->HexEdit, 8);
}

/**
 A callback invoked when the about menu item is invoked.

 @param Ctrl Pointer to the menu bar control.
 */
VOID
HexEditAboutButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    YORI_STRING Title;
    YORI_STRING Text;
    YORI_STRING LeftText;
    YORI_STRING CenteredText;
    YORI_STRING ButtonTexts[2];
    DWORD Index;
    DWORD ButtonClicked;

    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);

    YoriLibConstantString(&Title, _T("About"));
    YoriLibInitEmptyString(&Text);
    YoriLibYPrintf(&Text,
                   _T("HexEdit %i.%02i\n")
#if YORI_BUILD_ID
                   _T("Build %i\n")
#endif
                   _T("%hs"),

                   YORI_VER_MAJOR,
                   YORI_VER_MINOR,
#if YORI_BUILD_ID
                   YORI_BUILD_ID,
#endif
                   strHexEditHelpText);

    if (Text.StartOfString == NULL) {
        return;
    }

    //
    //  Search through the combined string to find the split point where
    //  earlier text should be centered and later text should be left
    //  aligned.  This is done to allow documentation for switches to be
    //  legible.  The split point is therefore defined as the first place
    //  a newline is followed by a space, indicating documentation for a
    //  switch.
    //
    //  Note the label control will swallow all leading spaces in a line.
    //

    YoriLibInitEmptyString(&CenteredText);
    YoriLibInitEmptyString(&LeftText);

    for (Index = 0; Index < Text.LengthInChars; Index++) {
        if (Text.StartOfString[Index] == '\n' &&
            Index + 1 < Text.LengthInChars &&
            Text.StartOfString[Index + 1] == ' ') {

            CenteredText.StartOfString = Text.StartOfString;
            CenteredText.LengthInChars = Index;

            LeftText.StartOfString = &Text.StartOfString[Index + 1];
            LeftText.LengthInChars = Text.LengthInChars - Index - 1;

            break;
        }
    }

    YoriLibConstantString(&ButtonTexts[0], _T("&Ok"));
    YoriLibConstantString(&ButtonTexts[1], _T("&View License..."));

    ButtonClicked = YoriDlgAbout(YoriWinGetWindowManagerHandle(Parent),
                                 &Title,
                                 &CenteredText,
                                 &LeftText,
                                 2,
                                 ButtonTexts,
                                 0,
                                 0);

    YoriLibFreeStringContents(&Text);

    if (ButtonClicked == 2) {
        if (YoriLibMitLicenseText(strHexEditCopyrightYear, &Text)) {

            YoriLibInitEmptyString(&CenteredText);
            YoriLibConstantString(&Title, _T("License"));

            //
            //  Replace all single line breaks with spaces but leave one line
            //  break in the case of double line (paragraph) breaks.  The
            //  label control can decide how to format lines.
            //

            for (Index = 0; Index < Text.LengthInChars; Index++) {
                if (Text.StartOfString[Index] == '\n' &&
                    Index + 1 < Text.LengthInChars &&
                    Text.StartOfString[Index + 1] != '\n') {

                    Text.StartOfString[Index] = ' ';
                }
            }

            YoriDlgAbout(YoriWinGetWindowManagerHandle(Parent),
                         &Title,
                         &CenteredText,
                         &Text,
                         1,
                         ButtonTexts,
                         0,
                         0);
            YoriLibFreeStringContents(&Text);
        }
    }
}

/**
 A callback from the multiline hexedit control to indicate the cursor has moved
 and the status bar should be updated.

 @param Ctrl Pointer to the multiline hexedit control.

 @param BufferOffset The offset within the buffer being edited.

 @param BitShift The number of bits shifted from the lowest order bit, used
        because when editing hex digits there are multiple display cells per
        byte.
 */
VOID
HexEditNotifyCursorMove(
    __in PYORI_WIN_CTRL_HANDLE Ctrl,
    __in DWORDLONG BufferOffset,
    __in DWORD BitShift
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PHEXEDIT_CONTEXT HexEditContext;
    YORI_STRING NewStatus;
    LARGE_INTEGER liBufferOffset;

    UNREFERENCED_PARAMETER(BitShift);

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    liBufferOffset.QuadPart = BufferOffset;
    YoriLibInitEmptyString(&NewStatus);
    YoriLibYPrintf(&NewStatus, _T("0x%08x`%08x "), liBufferOffset.HighPart, liBufferOffset.LowPart);

    YoriWinLabelSetCaption(HexEditContext->StatusBar, &NewStatus);
    YoriLibFreeStringContents(&NewStatus);

    //
    //  In a strange optimization reversal, force a repaint after this update
    //  in the hope that this console update is isolated to this without
    //  needing to update piles of user text at the same time
    //

    YoriWinDisplayWindowContents(Parent);
}


/**
 Create the menu bar and add initial items to it.

 @param HexEditContext Pointer to the hexedit context.

 @param Parent Handle to the main window.

 @return Pointer to the menu bar control if it was successfully created
         and populated, or NULL on failure.
 */
PYORI_WIN_CTRL_HANDLE
HexEditPopulateMenuBar(
    __in PHEXEDIT_CONTEXT HexEditContext,
    __in PYORI_WIN_WINDOW_HANDLE Parent
    )
{
    YORI_WIN_MENU_ENTRY FileMenuEntries[7];
    YORI_WIN_MENU_ENTRY SearchMenuEntries[1];
    YORI_WIN_MENU_ENTRY ViewMenuEntries[4];
    YORI_WIN_MENU_ENTRY HelpMenuEntries[1];
    YORI_WIN_MENU_ENTRY MenuEntries[4];
    YORI_WIN_MENU MenuBarItems;
    PYORI_WIN_CTRL_HANDLE Ctrl;
    DWORD MenuIndex;
    DWORD FileMenuCount;

    UNREFERENCED_PARAMETER(HexEditContext);

    ZeroMemory(&FileMenuEntries, sizeof(FileMenuEntries));
    MenuIndex = 0;
    YoriLibConstantString(&FileMenuEntries[MenuIndex].Caption, _T("&New"));
    YoriLibConstantString(&FileMenuEntries[MenuIndex].Hotkey, _T("Ctrl+N"));
    FileMenuEntries[MenuIndex].NotifyCallback = HexEditNewButtonClicked;
    MenuIndex++;

    YoriLibConstantString(&FileMenuEntries[MenuIndex].Caption, _T("&Open..."));
    YoriLibConstantString(&FileMenuEntries[MenuIndex].Hotkey, _T("Ctrl+O"));
    FileMenuEntries[MenuIndex].NotifyCallback = HexEditOpenButtonClicked;
    MenuIndex++;

    YoriLibConstantString(&FileMenuEntries[MenuIndex].Caption, _T("Open &Device..."));
    YoriLibConstantString(&FileMenuEntries[MenuIndex].Hotkey, _T("Ctrl+D"));
    FileMenuEntries[MenuIndex].NotifyCallback = HexEditOpenDeviceButtonClicked;
    MenuIndex++;

    YoriLibConstantString(&FileMenuEntries[MenuIndex].Caption, _T("&Save"));
    FileMenuEntries[MenuIndex].NotifyCallback = HexEditSaveButtonClicked;
    YoriLibConstantString(&FileMenuEntries[MenuIndex].Hotkey, _T("Ctrl+S"));
    MenuIndex++;

    YoriLibConstantString(&FileMenuEntries[MenuIndex].Caption, _T("Save &As..."));
    FileMenuEntries[MenuIndex].NotifyCallback = HexEditSaveAsButtonClicked;
    MenuIndex++;
    FileMenuEntries[MenuIndex].Flags = YORI_WIN_MENU_ENTRY_SEPERATOR;
    MenuIndex++;

    YoriLibConstantString(&FileMenuEntries[MenuIndex].Caption, _T("E&xit"));
    YoriLibConstantString(&FileMenuEntries[MenuIndex].Hotkey, _T("Ctrl+Q"));
    FileMenuEntries[MenuIndex].NotifyCallback = HexEditExitButtonClicked;
    MenuIndex++;

    FileMenuCount = MenuIndex;

    ZeroMemory(&SearchMenuEntries, sizeof(SearchMenuEntries));
    MenuIndex = 0;
    YoriLibConstantString(&SearchMenuEntries[MenuIndex].Caption, _T("&Go to..."));
    YoriLibConstantString(&SearchMenuEntries[MenuIndex].Hotkey, _T("Ctrl+G"));
    SearchMenuEntries[MenuIndex].NotifyCallback = HexEditGoToButtonClicked;

    ZeroMemory(&ViewMenuEntries, sizeof(ViewMenuEntries));
    MenuIndex = 0;
    YoriLibConstantString(&ViewMenuEntries[MenuIndex].Caption, _T("&Bytes"));
    YoriLibConstantString(&ViewMenuEntries[MenuIndex].Hotkey, _T("Ctrl+B"));
    ViewMenuEntries[MenuIndex].NotifyCallback = HexEditViewBytesButtonClicked;
    MenuIndex++;

    YoriLibConstantString(&ViewMenuEntries[MenuIndex].Caption, _T("&Words"));
    ViewMenuEntries[MenuIndex].NotifyCallback = HexEditViewWordsButtonClicked;
    MenuIndex++;

    YoriLibConstantString(&ViewMenuEntries[MenuIndex].Caption, _T("&DWords"));
    YoriLibConstantString(&ViewMenuEntries[MenuIndex].Hotkey, _T("Ctrl+D"));
    ViewMenuEntries[MenuIndex].NotifyCallback = HexEditViewDwordsButtonClicked;
    MenuIndex++;

    YoriLibConstantString(&ViewMenuEntries[MenuIndex].Caption, _T("&QWords"));
    ViewMenuEntries[MenuIndex].NotifyCallback = HexEditViewQwordsButtonClicked;
    MenuIndex++;

    ZeroMemory(&HelpMenuEntries, sizeof(HelpMenuEntries));
    MenuIndex = 0;
    YoriLibConstantString(&HelpMenuEntries[MenuIndex].Caption, _T("&About..."));
    HelpMenuEntries[MenuIndex].NotifyCallback = HexEditAboutButtonClicked;

    MenuBarItems.ItemCount = sizeof(MenuEntries)/sizeof(MenuEntries[0]);
    MenuBarItems.Items = MenuEntries;

    MenuIndex = 0;
    ZeroMemory(MenuEntries, sizeof(MenuEntries));
    YoriLibConstantString(&MenuEntries[MenuIndex].Caption, _T("&File"));
    MenuEntries[MenuIndex].ChildMenu.ItemCount = FileMenuCount;
    MenuEntries[MenuIndex].ChildMenu.Items = FileMenuEntries;
    MenuIndex++;

    YoriLibConstantString(&MenuEntries[MenuIndex].Caption, _T("&Search"));
    MenuEntries[MenuIndex].ChildMenu.ItemCount = sizeof(SearchMenuEntries)/sizeof(SearchMenuEntries[0]);
    MenuEntries[MenuIndex].ChildMenu.Items = SearchMenuEntries;
    MenuIndex++;

    YoriLibConstantString(&MenuEntries[MenuIndex].Caption, _T("&View"));
    MenuEntries[MenuIndex].ChildMenu.ItemCount = sizeof(ViewMenuEntries)/sizeof(ViewMenuEntries[0]);
    MenuEntries[MenuIndex].ChildMenu.Items = ViewMenuEntries;
    MenuIndex++;

    YoriLibConstantString(&MenuEntries[MenuIndex].Caption, _T("&Help"));
    MenuEntries[MenuIndex].ChildMenu.ItemCount = sizeof(HelpMenuEntries)/sizeof(HelpMenuEntries[0]);
    MenuEntries[MenuIndex].ChildMenu.Items = HelpMenuEntries;

    Ctrl = YoriWinMenuBarCreate(Parent, 0);
    if (Ctrl == NULL) {
        return NULL;
    }

    if (!YoriWinMenuBarAppendItems(Ctrl, &MenuBarItems)) {
        return NULL;
    }

    return Ctrl;
}

/**
 The minimum width in characters where hexedit can hope to function.
 */
#define HEXEDIT_MINIMUM_WIDTH (60)

/**
 The minimum height in characters where hexedit can hope to function.
 */
#define HEXEDIT_MINIMUM_HEIGHT (20)

/**
 A callback that is invoked when the window manager is being resized.  This
 typically means the user resized the window.  Since the purpose of hexedit is
 to fully occupy the window space, this implies the main window needs to be
 repositioned and/or resized, and the controls within it need to be
 repositioned and/or resized to full the window.

 @param WindowHandle Handle to the main window.

 @param OldPosition The old dimensions of the window manager.

 @param NewPosition The new dimensions of the window manager.
 */
VOID
HexEditResizeWindowManager(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle,
    __in PSMALL_RECT OldPosition,
    __in PSMALL_RECT NewPosition
    )
{
    PHEXEDIT_CONTEXT HexEditContext;
    PYORI_WIN_CTRL_HANDLE WindowCtrl;
    SMALL_RECT Rect;
    COORD NewSize;

    UNREFERENCED_PARAMETER(OldPosition);

    WindowCtrl = YoriWinGetCtrlFromWindow(WindowHandle);
    HexEditContext = YoriWinGetControlContext(WindowCtrl);

    NewSize.X = (SHORT)(NewPosition->Right - NewPosition->Left + 1);
    NewSize.Y = (SHORT)(NewPosition->Bottom - NewPosition->Top + 1);

    if (NewSize.X < HEXEDIT_MINIMUM_WIDTH || NewSize.Y < HEXEDIT_MINIMUM_HEIGHT) {
        return;
    }

    //
    //  Resize the main window, including capturing its new background
    //

    if (!YoriWinWindowReposition(WindowHandle, NewPosition)) {
        return;
    }

    //
    //  Reposition and resize child controls on the main window, causing them
    //  to redraw themselves
    //

    Rect.Left = 0;
    Rect.Top = 0;
    Rect.Right = (SHORT)(NewSize.X - 1);
    Rect.Bottom = 0;

    YoriWinMenuBarReposition(HexEditContext->MenuBar, &Rect);

    YoriWinGetClientSize(WindowHandle, &NewSize);

    Rect.Left = 0;
    Rect.Top = 0;
    Rect.Right = (SHORT)(NewSize.X - 1);
    Rect.Bottom = (SHORT)(NewSize.Y - 2);

    YoriWinHexEditReposition(HexEditContext->HexEdit, &Rect);

    Rect.Left = 0;
    Rect.Top = (SHORT)(NewSize.Y - 1);
    Rect.Right = (SHORT)(NewSize.X - 1);
    Rect.Bottom = (SHORT)(NewSize.Y - 1);

    YoriWinLabelReposition(HexEditContext->StatusBar, &Rect);
}


/**
 Display a popup window containing a list of items.

 @return TRUE to indicate that the user successfully selected an option, FALSE
         to indicate the menu could not be displayed or the user cancelled the
         operation.
 */
__success(return)
BOOL
HexEditCreateMainWindow(
    __in PHEXEDIT_CONTEXT HexEditContext
    )
{
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgr;
    PYORI_WIN_CTRL_HANDLE HexEdit;
    PYORI_WIN_WINDOW_HANDLE Parent;
    SMALL_RECT Rect;
    COORD WindowSize;
    PYORI_WIN_CTRL_HANDLE MenuBar;
    PYORI_WIN_CTRL_HANDLE StatusBar;
    DWORD_PTR Result;
    YORI_STRING Caption;

    if (!YoriWinOpenWindowManager(TRUE, &WinMgr)) {
        return FALSE;
    }

    if (HexEditContext->UseAsciiDrawing) {
        YoriWinMgrSetAsciiDrawing(WinMgr, HexEditContext->UseAsciiDrawing);
    }

    if (!YoriWinGetWinMgrDimensions(WinMgr, &WindowSize)) {
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    if (WindowSize.X < 60 || WindowSize.Y < 20) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("hexedit: window size too small\n"));
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    if (!YoriWinCreateWindow(WinMgr, WindowSize.X, WindowSize.Y, WindowSize.X, WindowSize.Y, 0, NULL, &Parent)) {
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    MenuBar = HexEditPopulateMenuBar(HexEditContext, Parent);
    if (MenuBar == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriWinGetClientSize(Parent, &WindowSize);

    Rect.Left = 0;
    Rect.Top = 0;
    Rect.Right = (SHORT)(WindowSize.X - 1);
    Rect.Bottom = (SHORT)(WindowSize.Y - 2);

    HexEdit = YoriWinHexEditCreate(Parent, NULL, &Rect, 1, YORI_WIN_HEX_EDIT_STYLE_VSCROLLBAR | YORI_WIN_HEX_EDIT_STYLE_LARGE_OFFSET);
    if (HexEdit == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    Rect.Top = (SHORT)(Rect.Bottom + 1);
    Rect.Bottom = Rect.Top;

    YoriLibInitEmptyString(&Caption);

    StatusBar = YoriWinLabelCreate(Parent, &Rect, &Caption, YORI_WIN_LABEL_STYLE_RIGHT_ALIGN);
    if (StatusBar == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    if (YoriLibDoesSystemSupportBackgroundColors()) {
        YoriWinLabelSetTextAttributes(StatusBar, BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
    }

    HexEditContext->WinMgr = WinMgr;
    HexEditContext->HexEdit = HexEdit;
    HexEditContext->MenuBar = MenuBar;
    HexEditContext->StatusBar = StatusBar;

    if (YoriLibDoesSystemSupportBackgroundColors()) {
        YoriWinHexEditSetColor(HexEdit,
                                     BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
                                     BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
    }
    YoriWinSetControlContext(Parent, HexEditContext);
    YoriWinHexEditSetCursorMoveNotifyCallback(HexEdit, HexEditNotifyCursorMove);

    YoriWinSetWindowManagerResizeNotifyCallback(Parent, HexEditResizeWindowManager);

    if (HexEditContext->OpenFileName.StartOfString != NULL) {
        HexEditLoadFile(HexEditContext, &HexEditContext->OpenFileName, 0, 0);
        HexEditUpdateOpenedFileCaption(HexEditContext);
        YoriWinHexEditSetReadOnly(HexEditContext->HexEdit, HexEditContext->ReadOnly);
    }

    Result = FALSE;
    if (!YoriWinProcessInputForWindow(Parent, &Result)) {
        Result = FALSE;
    }

    YoriWinDestroyWindow(Parent);
    YoriWinCloseWindowManager(WinMgr);
    return (BOOL)Result;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the hexedit builtin command.
 */
#define ENTRYPOINT YoriCmd_YHEXEDIT
#else
/**
 The main entrypoint for the hexedit standalone application.
 */
#define ENTRYPOINT ymain
#endif


/**
 Display yori shell hexeditor.

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
    DWORD Result;
    YORI_STRING Arg;

    ZeroMemory(&GlobalHexEditContext, sizeof(GlobalHexEditContext));

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                HexEditHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(strHexEditCopyrightYear);
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("a")) == 0) {
                GlobalHexEditContext.UseAsciiDrawing = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("r")) == 0) {
                GlobalHexEditContext.ReadOnly = TRUE;
                ArgumentUnderstood = TRUE;
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

    if (StartArg > 0 && StartArg < ArgC) {
        if (!YoriLibUserStringToSingleFilePath(&ArgV[StartArg], TRUE, &GlobalHexEditContext.OpenFileName)) {
            return EXIT_FAILURE;
        }
    }

    Result = EXIT_SUCCESS;

    if (!HexEditCreateMainWindow(&GlobalHexEditContext)) {
        Result = EXIT_FAILURE;
    }

#if !YORI_BUILTIN
    YoriLibLineReadCleanupCache();
#endif

    HexEditFreeHexEditContext(&GlobalHexEditContext);
    return Result;
}

// vim:sw=4:ts=4:et:
