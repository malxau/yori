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
        "HEXEDIT [-license] [-a] [-l|-n|-s] [-b|-w|-d|-q] [-r] [filename]\n"
        "\n"
        "   -a             Use ASCII characters for drawing\n"
        "   -b             Display as bytes\n"
        "   -d             Display as Dwords\n"
        "   -l             Display long offsets\n"
        "   -n             Display no offsets\n"
        "   -q             Display as Qwords\n"
        "   -r             Open file as read only\n"
        "   -s             Display short offsets\n"
        "   -w             Display as word\n";

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
    YORI_ALLOC_SIZE_T DataLength;

    /**
     The data that was most recently searched for.
     */
    PUCHAR SearchBuffer;

    /**
     The length of the data that was most recently searched for.
     */
    YORI_ALLOC_SIZE_T SearchBufferLength;

    /**
     The index of the edit menu.  This is used to check and uncheck menu
     items based on the state of the control.
     */
    DWORD EditMenuIndex;

    /**
     The index of the edit cut menu item.
     */
    DWORD EditCutMenuIndex;

    /**
     The index of the edit copy menu item.
     */
    DWORD EditCopyMenuIndex;

    /**
     The index of the edit paste menu item.
     */
    DWORD EditPasteMenuIndex;

    /**
     The index of the edit clear menu item.
     */
    DWORD EditClearMenuIndex;

    /**
     The index of the view menu.  This is used to check and uncheck menu
     items based on the state of the control.
     */
    DWORD ViewMenuIndex;

    /**
     The index of the view as bytes menu item.
     */
    DWORD ViewBytesMenuIndex;

    /**
     The index of the view as words menu item.
     */
    DWORD ViewWordsMenuIndex;

    /**
     The index of the view as double words menu item.
     */
    DWORD ViewDwordsMenuIndex;

    /**
     The index of the view as quad words menu item.
     */
    DWORD ViewQwordsMenuIndex;

    /**
     The index of the view no offset menu item.
     */
    DWORD ViewNoOffsetMenuIndex;

    /**
     The index of the view short offset menu item.
     */
    DWORD ViewShortOffsetMenuIndex;

    /**
     The index of the view long offset menu item.
     */
    DWORD ViewLongOffsetMenuIndex;

    /**
     Specifies the number of bytes per word.  Currently supported values are
     1, 2, 4 and 8.
     */
    UCHAR BytesPerWord;

    /**
     Specifies the number of bits to use for the buffer offset.  Currently
     supported values are 0, 32 and 64.
     */
    UCHAR OffsetWidth;

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
    if (HexEditContext->SearchBuffer) {
        YoriLibDereference(HexEditContext->SearchBuffer);
    }
}

/**
 Free a buffer, update its value to NULL and length to zero.

 @param Buffer Pointer to the pointer containing the buffer to free.  This
        will be updated within this routine.

 @param BufferLength Pointer to the length of the buffer.  This will be
        updated within this routine.
 */
VOID
HexEditFreeDataBuffer(
    __in PVOID * Buffer,
    __in PYORI_ALLOC_SIZE_T BufferLength
    )
{
    PVOID Data;
    Data = *Buffer;
    if (Data != NULL) {
        YoriLibDereference(Data);
        *Buffer = NULL;
    }

    *BufferLength = 0;
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
        NewCaption.LengthInChars = (YORI_ALLOC_SIZE_T)(HexEditContext->OpenFileName.LengthInChars - (FinalSlash - HexEditContext->OpenFileName.StartOfString + 1));
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
    YORI_MAX_SIGNED_T nFileSize;
    YORI_ALLOC_SIZE_T ReadLength;
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

    nFileSize = (YORI_MAX_SIGNED_T)FileSize.QuadPart;
    if (!YoriLibIsSizeAllocatable(nFileSize)) {
        CloseHandle(hFile);
        return ERROR_READ_FAULT;
    }

    ReadLength = (YORI_ALLOC_SIZE_T)FileSize.QuadPart;

    FileOffset.QuadPart = DataOffset;
    if (FileOffset.QuadPart != 0) {
        if (!SetFilePointer(hFile, FileOffset.LowPart, &FileOffset.HighPart, FILE_BEGIN)) {
            Err = GetLastError();
            CloseHandle(hFile);
            return Err;
        }
    }

    Buffer = YoriLibReferencedMalloc(ReadLength);
    if (Buffer == NULL) {
        CloseHandle(hFile);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (!ReadFile(hFile, Buffer, ReadLength, &BytesRead, NULL)) {
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

    if (BytesRead > ReadLength ||
        (DataLength != 0 && BytesRead != ReadLength)) {

        YoriLibDereference(Buffer);
        CloseHandle(hFile);
        return ERROR_INVALID_DATA;
    }

    CloseHandle(hFile);

    YoriWinHexEditClear(HexEditContext->HexEdit);

    if (!YoriWinHexEditSetDataNoCopy(HexEditContext->HexEdit, Buffer, ReadLength, (YORI_ALLOC_SIZE_T)BytesRead)) {
        YoriLibDereference(Buffer);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    YoriLibDereference(Buffer);

    HexEditContext->DataOffset = DataOffset;
    HexEditContext->DataLength = ReadLength;

    return ERROR_SUCCESS;
}

/**
 Save the contents of the opened window into a file.

 @param HexEditContext Pointer to the hexedit context.

 @param WinMgrHandle Handle to the window manager to use when displaying
        errors.

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
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_STRING FileName,
    __in DWORDLONG DataOffset,
    __in DWORDLONG DataLength
    )
{
    YORI_ALLOC_SIZE_T Index;
    DWORD Attributes;
    DWORD Err;
    LPTSTR ErrText;
    YORI_STRING ParentDirectory;
    YORI_STRING Prefix;
    YORI_STRING TempFileName;
    HANDLE WriteHandle;
    BOOLEAN ReplaceSucceeded;
    PUCHAR Buffer;
    YORI_ALLOC_SIZE_T BufferLength;
    YORI_ALLOC_SIZE_T EffectiveDataLength;
    DWORD BufferWritten;
    YORI_STRING Text;
    YORI_STRING Title;
    YORI_STRING ButtonText;

    YoriLibInitEmptyString(&TempFileName);
    YoriLibInitEmptyString(&Text);
    YoriLibConstantString(&Title, _T("Save"));
    YoriLibConstantString(&ButtonText, _T("Ok"));

    if (FileName->StartOfString == NULL) {
        YoriLibConstantString(&Text, _T("Cannot save: file name not specified"));
        goto DisplayErrorAndFail;
    }

    if (!YoriLibIsSizeAllocatable(DataLength)) {
        YoriLibConstantString(&Text, _T("Cannot save: device size too large"));
        goto DisplayErrorAndFail;
    }

    EffectiveDataLength = (YORI_ALLOC_SIZE_T)DataLength;

    ASSERT(YoriLibIsStringNullTerminated(FileName));

    if (!YoriLibIsFileNameDeviceName(FileName)) {

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

        if (!YoriLibGetTempFileName(&ParentDirectory, &Prefix, &WriteHandle, &TempFileName)) {

            YoriLibYPrintf(&Text, _T("Could not open temporary file in %y"), &ParentDirectory);
            goto DisplayErrorAndFail;
        }

    } else {

        WriteHandle = CreateFile(FileName->StartOfString,
                                 FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_FLAG_NO_BUFFERING,
                                 NULL);

        if (WriteHandle == INVALID_HANDLE_VALUE) {
            Err = GetLastError();
            ErrText = YoriLibGetWinErrorText(Err);
            YoriLibYPrintf(&Text, _T("Could not open device %s: %s"), FileName->StartOfString, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            goto DisplayErrorAndFail;
        }
    }

    YoriWinHexEditGetDataNoCopy(HexEditContext->HexEdit, &Buffer, &BufferLength);

    if (EffectiveDataLength != 0 &&
        EffectiveDataLength != BufferLength) {

        YoriLibYPrintf(&Text, _T("Device length %i bytes does not match buffer length %i bytes"), EffectiveDataLength, BufferLength);
        goto DisplayErrorAndFail;
    }

    if (DataOffset != 0) {
        LARGE_INTEGER FileOffset;
        FileOffset.QuadPart = DataOffset;
        if (!SetFilePointer(WriteHandle, FileOffset.LowPart, &FileOffset.HighPart, FILE_BEGIN)) {
            CloseHandle(WriteHandle);
            if (TempFileName.LengthInChars > 0) {
                DeleteFile(TempFileName.StartOfString);
            }
            YoriLibFreeStringContents(&TempFileName);
            YoriLibYPrintf(&Text, _T("Could not seek to offset 0x%llx"), FileOffset.QuadPart);
            goto DisplayErrorAndFail;
        }
    }

    if (Buffer != NULL) {

        //
        //  MSFIX This is truncating to 4Gb but is probably limited much lower
        //

        if (!WriteFile(WriteHandle, Buffer, (DWORD)BufferLength, &BufferWritten, NULL)) {
            Err = GetLastError();
            ErrText = YoriLibGetWinErrorText(Err);
            CloseHandle(WriteHandle);
            if (TempFileName.LengthInChars > 0) {
                DeleteFile(TempFileName.StartOfString);
            }
            YoriLibFreeStringContents(&TempFileName);
            YoriLibDereference(Buffer);
            YoriLibYPrintf(&Text, _T("Could not write to device: %s"), ErrText);
            YoriLibFreeWinErrorText(ErrText);
            goto DisplayErrorAndFail;
        }

        YoriLibDereference(Buffer);
    }

    if (TempFileName.LengthInChars > 0) {

        //
        //  Flush the temporary file to ensure it's durable, and rename it over
        //  the top of the chosen file, replacing if necessary.  This ensures
        //  that the old contents are not deleted until the new contents are
        //  successfully written.
        //

        if (!FlushFileBuffers(WriteHandle)) {
            CloseHandle(WriteHandle);
            DeleteFile(TempFileName.StartOfString);
            YoriLibFreeStringContents(&TempFileName);
            YoriLibConstantString(&Text, _T("Could not flush temporary file"));
            goto DisplayErrorAndFail;
        }

        CloseHandle(WriteHandle);

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
                YoriLibConstantString(&Text, _T("Could not replace file with temporary file"));
                goto DisplayErrorAndFail;
            }
            ReplaceSucceeded = TRUE;
        }

        if (!ReplaceSucceeded) {
            if (!MoveFileEx(TempFileName.StartOfString, FileName->StartOfString, MOVEFILE_REPLACE_EXISTING)) {
                DeleteFile(TempFileName.StartOfString);
                YoriLibFreeStringContents(&TempFileName);
                YoriLibConstantString(&Text, _T("Could not replace file with temporary file"));
                goto DisplayErrorAndFail;
            }
        }
    } else {
        CloseHandle(WriteHandle);
    }

    HexEditContext->DataOffset = DataOffset;
    HexEditContext->DataLength = EffectiveDataLength;

    YoriLibFreeStringContents(&TempFileName);
    return TRUE;

DisplayErrorAndFail:

    YoriDlgMessageBox(WinMgrHandle,
                      &Title,
                      &Text,
                      1,
                      &ButtonText,
                      0,
                      0);

    YoriLibFreeStringContents(&Text);

    return FALSE;
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
 Display a save as dialog.  The save may be for files or devices.

 @param Parent Handle to the main window.

 @param HexEditContext Pointer to the hexedit context.

 @param SaveDevice If TRUE, the save device dialog should be displayed.
        If FALSE, the save file dialog should be displayed.
 */
VOID
HexEditSaveAsDialog(
    __in PYORI_WIN_CTRL_HANDLE Parent,
    __in PHEXEDIT_CONTEXT HexEditContext,
    __in BOOLEAN SaveDevice
    )
{
    YORI_STRING Title;
    YORI_STRING Text;
    YORI_STRING FullName;
    DWORDLONG DeviceOffset;
    DWORDLONG DeviceLength;
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle;

    WinMgrHandle = YoriWinGetWindowManagerHandle(Parent);

    YoriLibConstantString(&Title, _T("Save As"));
    YoriLibInitEmptyString(&Text);

    DeviceOffset = 0;
    DeviceLength = 0;

    if (SaveDevice) {
        YoriDlgDevice(WinMgrHandle,
                      &Title,
                      0,
                      NULL,
                      &Text,
                      &DeviceOffset,
                      &DeviceLength);

    } else {
        YoriDlgFile(WinMgrHandle,
                    &Title,
                    0,
                    NULL,
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

    if (!HexEditSaveFile(HexEditContext, WinMgrHandle, &FullName, DeviceOffset, DeviceLength)) {
        YoriLibFreeStringContents(&FullName);
        return;
    }

    YoriLibFreeStringContents(&HexEditContext->OpenFileName);
    memcpy(&HexEditContext->OpenFileName, &FullName, sizeof(YORI_STRING));
    HexEditUpdateOpenedFileCaption(HexEditContext);
    YoriWinHexEditSetModifyState(HexEditContext->HexEdit, FALSE);
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
    PHEXEDIT_CONTEXT HexEditContext;

    PYORI_WIN_CTRL_HANDLE Parent;
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle;

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);
    WinMgrHandle = YoriWinGetWindowManagerHandle(Parent);

    if (HexEditContext->OpenFileName.StartOfString == NULL) {
        HexEditSaveAsButtonClicked(Ctrl);
        return;
    }

    if (!HexEditSaveFile(HexEditContext, WinMgrHandle, &HexEditContext->OpenFileName, HexEditContext->DataOffset, HexEditContext->DataLength)) {
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
    PHEXEDIT_CONTEXT HexEditContext;
    PYORI_WIN_CTRL_HANDLE Parent;

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    HexEditSaveAsDialog(Parent, HexEditContext, FALSE);
}

/**
 A callback invoked when the save as device menu item is invoked.

 @param Ctrl Pointer to the menu bar control.
 */
VOID
HexEditSaveAsDeviceButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PHEXEDIT_CONTEXT HexEditContext;
    PYORI_WIN_CTRL_HANDLE Parent;

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    HexEditSaveAsDialog(Parent, HexEditContext, TRUE);
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
 A callback invoked when the edit menu is opened.

 @param Ctrl Pointer to the menubar control.
 */
VOID
HexEditEditButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE EditMenu;
    PYORI_WIN_CTRL_HANDLE CutItem;
    PYORI_WIN_CTRL_HANDLE CopyItem;
    PYORI_WIN_CTRL_HANDLE PasteItem;
    PYORI_WIN_CTRL_HANDLE ClearItem;
    BOOLEAN DataSelected;
    PUCHAR ClipboardBuffer;
    YORI_ALLOC_SIZE_T ClipboardBufferLength;
    PYORI_WIN_CTRL_HANDLE Parent;
    PHEXEDIT_CONTEXT HexEditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    ClipboardBuffer = NULL;
    ClipboardBufferLength = 0;
    YoriLibPasteBinaryData(&ClipboardBuffer, &ClipboardBufferLength);

    DataSelected = YoriWinHexEditSelectionActive(HexEditContext->HexEdit);
    EditMenu = YoriWinMenuBarGetSubmenuHandle(Ctrl, NULL, HexEditContext->EditMenuIndex);
    CutItem = YoriWinMenuBarGetSubmenuHandle(Ctrl, EditMenu, HexEditContext->EditCutMenuIndex);
    CopyItem = YoriWinMenuBarGetSubmenuHandle(Ctrl, EditMenu, HexEditContext->EditCopyMenuIndex);
    PasteItem = YoriWinMenuBarGetSubmenuHandle(Ctrl, EditMenu, HexEditContext->EditPasteMenuIndex);
    ClearItem = YoriWinMenuBarGetSubmenuHandle(Ctrl, EditMenu, HexEditContext->EditClearMenuIndex);

    if (DataSelected) {
        YoriWinMenuBarEnableMenuItem(CutItem);
        YoriWinMenuBarEnableMenuItem(CopyItem);
        YoriWinMenuBarEnableMenuItem(ClearItem);
    } else {
        YoriWinMenuBarDisableMenuItem(CutItem);
        YoriWinMenuBarDisableMenuItem(CopyItem);
        YoriWinMenuBarDisableMenuItem(ClearItem);
    }

    if (ClipboardBuffer != NULL) {
        YoriWinMenuBarEnableMenuItem(PasteItem);
    } else {
        YoriWinMenuBarDisableMenuItem(PasteItem);
    }

    if (ClipboardBuffer != NULL) {
        YoriLibDereference(ClipboardBuffer);
    }
}

/**
 A callback invoked when the cut button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
HexEditCutButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PHEXEDIT_CONTEXT HexEditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    YoriWinHexEditCutSelectedData(HexEditContext->HexEdit);
}

/**
 A callback invoked when the copy button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
HexEditCopyButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PHEXEDIT_CONTEXT HexEditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    YoriWinHexEditCopySelectedData(HexEditContext->HexEdit);
}

/**
 A callback invoked when the paste button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
HexEditPasteButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PHEXEDIT_CONTEXT HexEditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    YoriWinHexEditPasteData(HexEditContext->HexEdit);
}

/**
 A callback invoked when the clear button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
HexEditClearButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PHEXEDIT_CONTEXT HexEditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);
    YoriWinHexEditDeleteSelection(HexEditContext->HexEdit);
    YoriWinHexEditSetModifyState(HexEditContext->HexEdit, TRUE);
}


/**
 Search forward through a memory buffer looking for a matching sub-buffer.
 Both are treated as opaque binary buffers.

 @param Buffer Pointer to the master buffer that may contain a match.

 @param BufferLength The length of the master buffer, in bytes.

 @param BufferOffset The initial offset to search within the master buffer,
        in bytes.

 @param SearchBuffer Pointer to the buffer to search for.

 @param SearchBufferLength The length of the search buffer, in bytes.

 @param FoundOffset On successful completion (ie., a match is found), updated
        to point to the offset within the master buffer of the match.

 @return TRUE to indicate a match was found, FALSE if no match was found.
 */
__success(return)
BOOLEAN
HexEditFindNextMemorySubset(
    __in PUCHAR Buffer,
    __in YORI_ALLOC_SIZE_T BufferLength,
    __in YORI_ALLOC_SIZE_T BufferOffset,
    __in PUCHAR SearchBuffer,
    __in YORI_ALLOC_SIZE_T SearchBufferLength,
    __out PYORI_ALLOC_SIZE_T FoundOffset
    )
{
    YORI_ALLOC_SIZE_T BufferIndex;
    YORI_ALLOC_SIZE_T SearchBufferIndex;
    YORI_ALLOC_SIZE_T EndIndex;

    if (BufferOffset > BufferLength ||
        BufferLength - BufferOffset < SearchBufferLength) {

        return FALSE;
    }

    EndIndex = BufferLength - SearchBufferLength + 1;

    for (BufferIndex = BufferOffset; BufferIndex < EndIndex; BufferIndex++) {
        for (SearchBufferIndex = 0; SearchBufferIndex < SearchBufferLength; SearchBufferIndex++) {
            if (Buffer[BufferIndex + SearchBufferIndex] != SearchBuffer[SearchBufferIndex]) {
                break;
            }
        }

        if (SearchBufferIndex == SearchBufferLength) {
            *FoundOffset = BufferIndex;
            return TRUE;
        }
    }

    return FALSE;
}

/**
 Search backward through a memory buffer looking for a matching sub-buffer.
 Both are treated as opaque binary buffers.

 @param Buffer Pointer to the master buffer that may contain a match.

 @param BufferLength The length of the master buffer, in bytes.

 @param BufferOffset The initial offset to search within the master buffer,
        in bytes.

 @param SearchBuffer Pointer to the buffer to search for.

 @param SearchBufferLength The length of the search buffer, in bytes.

 @param FoundOffset On successful completion (ie., a match is found), updated
        to point to the offset within the master buffer of the match.

 @return TRUE to indicate a match was found, FALSE if no match was found.
 */
__success(return)
BOOLEAN
HexEditFindPreviousMemorySubset(
    __in PUCHAR Buffer,
    __in YORI_ALLOC_SIZE_T BufferLength,
    __in YORI_ALLOC_SIZE_T BufferOffset,
    __in PUCHAR SearchBuffer,
    __in YORI_ALLOC_SIZE_T SearchBufferLength,
    __out PYORI_ALLOC_SIZE_T FoundOffset
    )
{
    YORI_ALLOC_SIZE_T BufferIndex;
    YORI_ALLOC_SIZE_T SearchBufferIndex;

    if (BufferOffset > BufferLength ||
        BufferLength - BufferOffset < SearchBufferLength) {

        return FALSE;
    }

    for (BufferIndex = BufferOffset; TRUE; BufferIndex--) {
        for (SearchBufferIndex = 0; SearchBufferIndex < SearchBufferLength; SearchBufferIndex++) {
            if (Buffer[BufferIndex + SearchBufferIndex] != SearchBuffer[SearchBufferIndex]) {
                break;
            }
        }

        if (SearchBufferIndex == SearchBufferLength) {
            *FoundOffset = BufferIndex;
            return TRUE;
        }

        if (BufferIndex == 0) {
            break;
        }
    }

    return FALSE;
}

/**
 Translate a byte aligned offset into a control buffer offset and bit shift.
 This is necessary because the control can display values of different word
 sizes, which is expressed using a buffer offset to point to the word and
 a bit shift to refer to the digit within the word.  Here we only care about
 being 8 bit aligned (no nibble alignment.)

 @param HexEditContext Pointer to the hexedit context, implicitly containing
        information about the bytes per word.

 @param ByteOffset The offset within the buffer, in bytes.

 @param BufferOffset On successful completion, updated to point to the
        beginning of the word within the buffer.

 @param BitShift On successful completion, updated to indicate the number
        of bits to shift to point to the digit describing the specified byte.
 */
VOID
HexEditByteOffsetToBufferOffsetAndShift(
    __in PHEXEDIT_CONTEXT HexEditContext,
    __in YORI_ALLOC_SIZE_T ByteOffset,
    __out PYORI_ALLOC_SIZE_T BufferOffset,
    __out PUCHAR BitShift
    )
{
    YORI_ALLOC_SIZE_T LocalBufferOffset;
    YORI_ALLOC_SIZE_T BytesPerWord;

    BytesPerWord = HexEditContext->BytesPerWord;
    BytesPerWord = (YORI_ALLOC_SIZE_T)(~(BytesPerWord - 1));

    LocalBufferOffset = (YORI_ALLOC_SIZE_T)(ByteOffset & BytesPerWord);
    *BufferOffset = LocalBufferOffset;
    *BitShift = (UCHAR)((ByteOffset - LocalBufferOffset) * 8);
}

/**
 Find the next search match from a specified byte offset.

 @param HexEditContext Pointer to the hexedit context, implicitly containing
        the buffer to search.

 @param StartOffset The byte offset to search within the buffer.

 @param MatchOffset If a new match is found, updated to contain the offset of
        the newly found match.

 @return TRUE to indicate a match was found, FALSE if no match was found.
 */
__success(return)
BOOLEAN
HexEditFindNextFromPosition(
    __in PHEXEDIT_CONTEXT HexEditContext,
    __in YORI_ALLOC_SIZE_T StartOffset,
    __out PYORI_ALLOC_SIZE_T MatchOffset
    )
{
    PUCHAR Buffer;
    YORI_ALLOC_SIZE_T BufferLength;
    YORI_ALLOC_SIZE_T FindOffset;

    YoriWinHexEditGetDataNoCopy(HexEditContext->HexEdit, &Buffer, &BufferLength);

    //
    //  This can happen if the hex edit control contains no data.  In that
    //  case, no match is found.
    //

    if (Buffer == NULL) {
        return FALSE;
    }

    if (StartOffset >= BufferLength) {
        YoriLibDereference(Buffer);
        return FALSE;
    }

    if (HexEditFindNextMemorySubset(Buffer, BufferLength, StartOffset, HexEditContext->SearchBuffer, HexEditContext->SearchBufferLength, &FindOffset)) {
        *MatchOffset = FindOffset;
        YoriLibDereference(Buffer);
        return TRUE;
    }

    YoriLibDereference(Buffer);
    return FALSE;
}

/**
 Find the next search match from the cursor position.

 @param HexEditContext Pointer to the hexedit context, implicitly containing
        the buffer to search and a cursor offset.

 @param StartAtNextByte TRUE to indicate searching should start from the byte
        after the cursor, FALSE if it should start at the cursor.

 @return TRUE to indicate a match was found, FALSE if no match was found.
 */
BOOLEAN
HexEditFindNextFromCurrentPosition(
    __in PHEXEDIT_CONTEXT HexEditContext,
    __in BOOLEAN StartAtNextByte
    )
{
    YORI_ALLOC_SIZE_T BufferOffset;
    YORI_ALLOC_SIZE_T FindOffset;
    UCHAR BitShift;
    BOOLEAN AsChar;

    if (!YoriWinHexEditGetCursorLocation(HexEditContext->HexEdit, &AsChar, &BufferOffset, &BitShift)) {
        return FALSE;
    }

    BufferOffset = BufferOffset + (BitShift / 8);
    if (StartAtNextByte) {
        BufferOffset = BufferOffset + 1;
    }

    if (HexEditFindNextFromPosition(HexEditContext, BufferOffset, &FindOffset)) {
        HexEditByteOffsetToBufferOffsetAndShift(HexEditContext, FindOffset, &BufferOffset, &BitShift);
        YoriWinHexEditSetCursorLocation(HexEditContext->HexEdit, FALSE, BufferOffset, BitShift);
        YoriWinHexEditSetSelectionRange(HexEditContext->HexEdit, FindOffset, FindOffset + HexEditContext->SearchBufferLength - 1);
        return TRUE;
    }

    return FALSE;
}

/**
 Find the previous search match from the cursor position.

 @param HexEditContext Pointer to the hexedit context, implicitly containing
        the buffer to search and a cursor offset.

 @return TRUE to indicate a match was found, FALSE if no match was found.
 */
BOOLEAN
HexEditFindPreviousFromCurrentPosition(
    __in PHEXEDIT_CONTEXT HexEditContext
    )
{
    PUCHAR Buffer;
    YORI_ALLOC_SIZE_T BufferLength;
    YORI_ALLOC_SIZE_T BufferOffset;
    UCHAR BitShift;
    BOOLEAN AsChar;
    YORI_ALLOC_SIZE_T FindOffset;

    YoriWinHexEditGetDataNoCopy(HexEditContext->HexEdit, &Buffer, &BufferLength);

    //
    //  This can happen if the hex edit control contains no data.  In that
    //  case, no match is found.
    //

    if (Buffer == NULL) {
        return FALSE;
    }

    if (!YoriWinHexEditGetCursorLocation(HexEditContext->HexEdit, &AsChar, &BufferOffset, &BitShift)) {
        YoriLibDereference(Buffer);
        return FALSE;
    }

    BufferOffset = BufferOffset + (BitShift / 8);

    if (BufferOffset == 0) {
        YoriLibDereference(Buffer);
        return FALSE;
    }

    BufferOffset = BufferOffset - 1;

    if (HexEditFindPreviousMemorySubset(Buffer, BufferLength, BufferOffset, HexEditContext->SearchBuffer, HexEditContext->SearchBufferLength, &FindOffset)) {
        HexEditByteOffsetToBufferOffsetAndShift(HexEditContext, FindOffset, &BufferOffset, &BitShift);
        YoriWinHexEditSetCursorLocation(HexEditContext->HexEdit, FALSE, BufferOffset, BitShift);
        YoriWinHexEditSetSelectionRange(HexEditContext->HexEdit, FindOffset, FindOffset + HexEditContext->SearchBufferLength - 1);
        YoriLibDereference(Buffer);
        return TRUE;
    }

    YoriLibDereference(Buffer);
    return FALSE;
}

/**
 A callback invoked when the find menu item is invoked.

 @param Ctrl Pointer to the menu bar control.
 */
VOID
HexEditFindButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    YORI_STRING Title;
    PYORI_WIN_CTRL_HANDLE Parent;
    PHEXEDIT_CONTEXT HexEditContext;
    PUCHAR FindData;
    YORI_ALLOC_SIZE_T FindDataLength;

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    YoriLibConstantString(&Title, _T("Find"));

    if (!YoriDlgFindHex(YoriWinGetWindowManagerHandle(Parent),
                        &Title,
                        HexEditContext->SearchBuffer,
                        HexEditContext->SearchBufferLength,
                        HexEditContext->BytesPerWord,
                        &FindData,
                        &FindDataLength)) {

        return;
    }

    if (FindData == NULL) {
        return;
    }

    if (HexEditContext->SearchBuffer != NULL) {
        YoriLibDereference(HexEditContext->SearchBuffer);
    }

    HexEditContext->SearchBuffer = FindData;
    HexEditContext->SearchBufferLength = FindDataLength;
    if (!HexEditFindNextFromCurrentPosition(HexEditContext, FALSE)) {
        YORI_STRING ButtonText[1];
        YORI_STRING Text;

        YoriLibConstantString(&Title, _T("Find"));
        YoriLibConstantString(&Text, _T("Data not found."));
        YoriLibConstantString(&ButtonText[0], _T("&Ok"));

        YoriDlgMessageBox(YoriWinGetWindowManagerHandle(Parent),
                          &Title,
                          &Text,
                          1,
                          ButtonText,
                          0,
                          0);
    }
}

/**
 A callback invoked when the repeat last find menu item is invoked.

 @param Ctrl Pointer to the menu bar control.
 */
VOID
HexEditFindNextButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PHEXEDIT_CONTEXT HexEditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    if (HexEditContext->SearchBuffer == NULL ||
        HexEditContext->SearchBufferLength == 0) {

        return;
    }

    if (!HexEditFindNextFromCurrentPosition(HexEditContext, TRUE)) {
        YORI_STRING Title;
        YORI_STRING Text;
        YORI_STRING ButtonText[1];

        YoriLibConstantString(&Title, _T("Find"));
        YoriLibConstantString(&Text, _T("No more matches found."));
        YoriLibConstantString(&ButtonText[0], _T("&Ok"));

        YoriDlgMessageBox(YoriWinGetWindowManagerHandle(Parent),
                          &Title,
                          &Text,
                          1,
                          ButtonText,
                          0,
                          0);
    }
}

/**
 A callback invoked when the find previous menu item is invoked.

 @param Ctrl Pointer to the menu bar control.
 */
VOID
HexEditFindPreviousButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PHEXEDIT_CONTEXT HexEditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    if (HexEditContext->SearchBuffer == NULL ||
        HexEditContext->SearchBufferLength == 0) {

        return;
    }

    if (!HexEditFindPreviousFromCurrentPosition(HexEditContext)) {
        YORI_STRING Title;
        YORI_STRING Text;
        YORI_STRING ButtonText[1];

        YoriLibConstantString(&Title, _T("Find"));
        YoriLibConstantString(&Text, _T("No more matches found."));
        YoriLibConstantString(&ButtonText[0], _T("&Ok"));

        YoriDlgMessageBox(YoriWinGetWindowManagerHandle(Parent),
                          &Title,
                          &Text,
                          1,
                          ButtonText,
                          0,
                          0);
    }
}

/**
 A callback invoked when the change menu item is invoked.

 @param Ctrl Pointer to the menu bar control.
 */
VOID
HexEditChangeButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgr;
    YORI_STRING Title;
    BOOLEAN ReplaceAll;
    BOOLEAN MatchFound;
    BOOLEAN AsChar;
    PYORI_WIN_CTRL_HANDLE Parent;
    PHEXEDIT_CONTEXT HexEditContext;
    YORI_ALLOC_SIZE_T StartOffset;
    YORI_ALLOC_SIZE_T NextMatchOffset;
    UCHAR BitShift;
    WORD DialogTop;

    PUCHAR InitialOldData;
    YORI_ALLOC_SIZE_T InitialOldDataLength;
    PUCHAR InitialNewData;
    YORI_ALLOC_SIZE_T InitialNewDataLength;
    PUCHAR OldData;
    YORI_ALLOC_SIZE_T OldDataLength;
    PUCHAR NewData;
    YORI_ALLOC_SIZE_T NewDataLength;

    InitialOldData = NULL;
    InitialOldDataLength = 0;
    InitialNewData = NULL;
    InitialNewDataLength = 0;
    OldData = NULL;
    OldDataLength = 0;
    NewData = NULL;
    NewDataLength = 0;
    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);
    WinMgr = YoriWinGetWindowManagerHandle(Parent);
    ReplaceAll = FALSE;
    MatchFound = FALSE;

    YoriWinHexEditGetCursorLocation(HexEditContext->HexEdit, &AsChar, &StartOffset, &BitShift);

    while(TRUE) {

        if (!ReplaceAll) {

            YoriLibConstantString(&Title, _T("Find"));

            //
            //  Populate the dialog with whatever is selected now, if anything
            //

            InitialOldData = NULL;
            InitialOldDataLength = 0;
            if (OldDataLength > 0) {
                YoriLibReference(OldData);
                InitialOldData = OldData;
                InitialOldDataLength = OldDataLength;
            } else {
                if (YoriWinHexEditSelectionActive(HexEditContext->HexEdit)) {
                    if (!YoriWinHexEditGetSelectedData(HexEditContext->HexEdit, &InitialOldData, &InitialOldDataLength)) {
                        InitialOldData = NULL;
                        InitialOldDataLength = 0;
                    }
                }
            }

            //
            //  Position the viewport so that the selection appears below the
            //  dialog.
            //

            DialogTop = (WORD)-1;
            if (MatchFound && !ReplaceAll) {

                COORD WinMgrSize;
                COORD ClientSize;
                YORI_ALLOC_SIZE_T CursorLine;
                YORI_ALLOC_SIZE_T CursorOffset;
                YORI_ALLOC_SIZE_T ViewportLeft;
                YORI_ALLOC_SIZE_T ViewportTop;
                WORD DialogHeight;
                WORD RemainingEditHeight;

                if (!YoriWinGetWinMgrDimensions(WinMgr, &WinMgrSize)) {
                    HexEditFreeDataBuffer(&InitialOldData, &InitialOldDataLength);
                    HexEditFreeDataBuffer(&InitialNewData, &InitialNewDataLength);
                    HexEditFreeDataBuffer(&OldData, &OldDataLength);
                    HexEditFreeDataBuffer(&NewData, &NewDataLength);
                    break;
                }
                DialogHeight = YoriDlgReplaceHexGetDialogHeight(WinMgr);
                DialogTop = (SHORT)(WinMgrSize.Y - DialogHeight - 1);

                YoriWinGetControlClientSize(HexEditContext->HexEdit, &ClientSize);
                YoriWinHexEditGetVisualCursorLocation(HexEditContext->HexEdit, &CursorOffset, &CursorLine);
                YoriWinHexEditGetViewportLocation(HexEditContext->HexEdit, &ViewportLeft, &ViewportTop);

                RemainingEditHeight = (SHORT)(ClientSize.Y - DialogHeight);

                if (CursorLine > (DWORD)(ViewportTop + RemainingEditHeight - 1)) {
                    ViewportTop = CursorLine - (RemainingEditHeight / 2);
                    YoriWinHexEditSetViewportLocation(HexEditContext->HexEdit, ViewportLeft, ViewportTop);
                }

                //
                //  When replacing one instance, make sure the user can see
                //  the highlighted text since normal window message
                //  processing isn't happening while we're looping displaying
                //  dialogs.  When replacing everything updating the display
                //  is just overhead.
                //

                YoriWinDisplayWindowContents(Parent);
            }

            InitialNewData = NewData;
            InitialNewDataLength = NewDataLength;
            NewData = NULL;
            NewDataLength = 0;

            HexEditFreeDataBuffer(&OldData, &OldDataLength);

            if (!YoriDlgReplaceHex(YoriWinGetWindowManagerHandle(Parent),
                                   (WORD)-1,
                                   DialogTop,
                                   &Title,
                                   InitialOldData,
                                   InitialOldDataLength,
                                   InitialNewData,
                                   InitialNewDataLength,
                                   HexEditContext->BytesPerWord,
                                   &ReplaceAll,
                                   &OldData,
                                   &OldDataLength,
                                   &NewData,
                                   &NewDataLength)) {

                HexEditFreeDataBuffer(&InitialOldData, &InitialOldDataLength);
                HexEditFreeDataBuffer(&InitialNewData, &InitialNewDataLength);
                HexEditFreeDataBuffer(&OldData, &OldDataLength);
                HexEditFreeDataBuffer(&NewData, &NewDataLength);
                break;
            }

            HexEditFreeDataBuffer(&InitialOldData, &InitialOldDataLength);
            HexEditFreeDataBuffer(&InitialNewData, &InitialNewDataLength);

            if (OldDataLength == 0) {
                HexEditFreeDataBuffer(&OldData, &OldDataLength);
                HexEditFreeDataBuffer(&NewData, &NewDataLength);
                return;
            }

            HexEditFreeDataBuffer(&HexEditContext->SearchBuffer, &HexEditContext->SearchBufferLength);

            YoriLibReference(OldData);
            HexEditContext->SearchBuffer = OldData;
            HexEditContext->SearchBufferLength = OldDataLength;
        }

        if (MatchFound) {
            YoriWinHexEditClearSelection(HexEditContext->HexEdit);
            YoriWinHexEditDeleteData(HexEditContext->HexEdit, StartOffset, OldDataLength);
            YoriWinHexEditInsertData(HexEditContext->HexEdit, StartOffset, NewData, NewDataLength);
            YoriWinHexEditSetModifyState(HexEditContext->HexEdit, TRUE);
            StartOffset = StartOffset + NewDataLength;
        }

        if (!HexEditFindNextFromPosition(HexEditContext, StartOffset, &NextMatchOffset)) {
            break;
        }

        MatchFound = TRUE;

        //
        //  MSFIX: In the ReplaceAll case, this is still updating the off
        //  screen buffer constantly.  Ideally this wouldn't happen, but
        //  it still needs to be updated once before returning to the user.
        //

        YoriWinHexEditSetSelectionRange(HexEditContext->HexEdit, NextMatchOffset, NextMatchOffset + NewDataLength - 1);
        YoriWinHexEditSetCursorLocation(HexEditContext->HexEdit, FALSE, NextMatchOffset, 0);
        StartOffset = NextMatchOffset;
    }

    HexEditFreeDataBuffer(&OldData, &OldDataLength);
    HexEditFreeDataBuffer(&NewData, &NewDataLength);
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
    YORI_MAX_SIGNED_T SignedNewOffset;
    YORI_MAX_UNSIGNED_T NewOffset;
    YORI_ALLOC_SIZE_T CharsConsumed;

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

    if (YoriLibStringToNumber(&Text, FALSE, &SignedNewOffset, &CharsConsumed) &&
        CharsConsumed > 0) {

        if (SignedNewOffset < 0) {
            NewOffset = 0;
        } else {
            NewOffset = (YORI_MAX_UNSIGNED_T)SignedNewOffset;
        }

        YoriWinHexEditSetCursorLocation(HexEditContext->HexEdit, FALSE, (YORI_ALLOC_SIZE_T)NewOffset, 0);
    }

    YoriLibFreeStringContents(&Text);
}

/**
 A callback invoked when the view button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
HexEditViewButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PHEXEDIT_CONTEXT HexEditContext;
    PYORI_WIN_CTRL_HANDLE ViewMenu;
    PYORI_WIN_CTRL_HANDLE ViewBytesItem;
    PYORI_WIN_CTRL_HANDLE ViewWordsItem;
    PYORI_WIN_CTRL_HANDLE ViewDwordsItem;
    PYORI_WIN_CTRL_HANDLE ViewQwordsItem;
    PYORI_WIN_CTRL_HANDLE ViewNoOffsetItem;
    PYORI_WIN_CTRL_HANDLE ViewShortOffsetItem;
    PYORI_WIN_CTRL_HANDLE ViewLongOffsetItem;

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    ViewMenu = YoriWinMenuBarGetSubmenuHandle(Ctrl, NULL, HexEditContext->ViewMenuIndex);
    ViewBytesItem = YoriWinMenuBarGetSubmenuHandle(Ctrl, ViewMenu, HexEditContext->ViewBytesMenuIndex);
    ViewWordsItem = YoriWinMenuBarGetSubmenuHandle(Ctrl, ViewMenu, HexEditContext->ViewWordsMenuIndex);
    ViewDwordsItem = YoriWinMenuBarGetSubmenuHandle(Ctrl, ViewMenu, HexEditContext->ViewDwordsMenuIndex);
    ViewQwordsItem = YoriWinMenuBarGetSubmenuHandle(Ctrl, ViewMenu, HexEditContext->ViewQwordsMenuIndex);
    ViewNoOffsetItem = YoriWinMenuBarGetSubmenuHandle(Ctrl, ViewMenu, HexEditContext->ViewNoOffsetMenuIndex);
    ViewShortOffsetItem = YoriWinMenuBarGetSubmenuHandle(Ctrl, ViewMenu, HexEditContext->ViewShortOffsetMenuIndex);
    ViewLongOffsetItem = YoriWinMenuBarGetSubmenuHandle(Ctrl, ViewMenu, HexEditContext->ViewLongOffsetMenuIndex);

    YoriWinMenuBarUncheckMenuItem(ViewBytesItem);
    YoriWinMenuBarUncheckMenuItem(ViewWordsItem);
    YoriWinMenuBarUncheckMenuItem(ViewDwordsItem);
    YoriWinMenuBarUncheckMenuItem(ViewQwordsItem);

    YoriWinMenuBarUncheckMenuItem(ViewNoOffsetItem);
    YoriWinMenuBarUncheckMenuItem(ViewShortOffsetItem);
    YoriWinMenuBarUncheckMenuItem(ViewLongOffsetItem);

    switch(HexEditContext->BytesPerWord) {
        case 1:
            YoriWinMenuBarCheckMenuItem(ViewBytesItem);
            break;
        case 2:
            YoriWinMenuBarCheckMenuItem(ViewWordsItem);
            break;
        case 4:
            YoriWinMenuBarCheckMenuItem(ViewDwordsItem);
            break;
        case 8:
            YoriWinMenuBarCheckMenuItem(ViewQwordsItem);
            break;
    }

    switch(HexEditContext->OffsetWidth) {
        case 0:
            YoriWinMenuBarCheckMenuItem(ViewNoOffsetItem);
            break;
        case 32:
            YoriWinMenuBarCheckMenuItem(ViewShortOffsetItem);
            break;
        case 64:
            YoriWinMenuBarCheckMenuItem(ViewLongOffsetItem);
            break;
    }
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

    HexEditContext->BytesPerWord = 1;
    YoriWinHexEditSetBytesPerWord(HexEditContext->HexEdit, HexEditContext->BytesPerWord);
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

    HexEditContext->BytesPerWord = 2;
    YoriWinHexEditSetBytesPerWord(HexEditContext->HexEdit, HexEditContext->BytesPerWord);
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

    HexEditContext->BytesPerWord = 4;
    YoriWinHexEditSetBytesPerWord(HexEditContext->HexEdit, HexEditContext->BytesPerWord);
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

    HexEditContext->BytesPerWord = 8;
    YoriWinHexEditSetBytesPerWord(HexEditContext->HexEdit, HexEditContext->BytesPerWord);
}

/**
 A callback invoked when the no offset button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
HexEditViewNoOffsetButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PHEXEDIT_CONTEXT HexEditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    if (YoriWinHexEditSetStyle(HexEditContext->HexEdit, 0)) {
        HexEditContext->OffsetWidth = 0;
    }
}

/**
 A callback invoked when the short offset button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
HexEditViewShortOffsetButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PHEXEDIT_CONTEXT HexEditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    if (YoriWinHexEditSetStyle(HexEditContext->HexEdit, YORI_WIN_HEX_EDIT_STYLE_OFFSET)) {
        HexEditContext->OffsetWidth = 32;
    }
}

/**
 A callback invoked when the long offset button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
HexEditViewLongOffsetButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PHEXEDIT_CONTEXT HexEditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    if (YoriWinHexEditSetStyle(HexEditContext->HexEdit, YORI_WIN_HEX_EDIT_STYLE_LARGE_OFFSET)) {
        HexEditContext->OffsetWidth = 64;
    }
}

/**
 A callback invoked when the calculate PE checksum menu item is invoked.

 @param Ctrl Pointer to the menu bar control.
 */
VOID
HexEditCalculatePEChecksumButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PHEXEDIT_CONTEXT HexEditContext;
    YORI_STRING Title;
    YORI_STRING Text;
    YORI_STRING ButtonText[1];
    PYORILIB_PE_HEADERS PeHeaders;
    PUCHAR Buffer;
    YORI_ALLOC_SIZE_T BufferLength;
    DWORD CurrentChecksum;
    DWORD NewChecksum;
    YORI_ALLOC_SIZE_T DataOffset;


    Parent = YoriWinGetControlParent(Ctrl);
    HexEditContext = YoriWinGetControlContext(Parent);

    YoriLibLoadImageHlpFunctions();

    if (DllImageHlp.pCheckSumMappedFile == NULL) {
        YoriLibConstantString(&Title, _T("Error"));
        YoriLibConstantString(&Text, _T("OS support not present"));
        YoriLibConstantString(&ButtonText[0], _T("&Ok"));

        YoriDlgMessageBox(YoriWinGetWindowManagerHandle(Parent),
                          &Title,
                          &Text,
                          1,
                          ButtonText,
                          0,
                          0);

        return;
    }

    if (!YoriWinHexEditGetDataNoCopy(HexEditContext->HexEdit, &Buffer, &BufferLength)) {
        return;
    }

    PeHeaders = DllImageHlp.pCheckSumMappedFile(Buffer, BufferLength, &CurrentChecksum, &NewChecksum);
    if (PeHeaders == NULL) {
        YoriLibDereference(Buffer);
        YoriLibConstantString(&Title, _T("Error"));
        YoriLibConstantString(&Text, _T("Could not calculate checksum.  Possibly not PE file?"));
        YoriLibConstantString(&ButtonText[0], _T("&Ok"));

        YoriDlgMessageBox(YoriWinGetWindowManagerHandle(Parent),
                          &Title,
                          &Text,
                          1,
                          ButtonText,
                          0,
                          0);
        return;
    }


    DataOffset = (YORI_ALLOC_SIZE_T)((PUCHAR)PeHeaders - Buffer);
    DataOffset = DataOffset + FIELD_OFFSET(YORILIB_PE_HEADERS, OptionalHeader.CheckSum);
    YoriLibDereference(Buffer);
    YoriWinHexEditReplaceData(HexEditContext->HexEdit, DataOffset, &NewChecksum, sizeof(NewChecksum));
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
    YORI_ALLOC_SIZE_T Index;
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
    YORI_WIN_MENU_ENTRY FileMenuEntries[8];
    YORI_WIN_MENU_ENTRY EditMenuEntries[4];
    YORI_WIN_MENU_ENTRY SearchMenuEntries[6];
    YORI_WIN_MENU_ENTRY ViewMenuEntries[8];
    YORI_WIN_MENU_ENTRY ToolsMenuEntries[1];
    YORI_WIN_MENU_ENTRY HelpMenuEntries[1];
    YORI_WIN_MENU_ENTRY MenuEntries[6];
    YORI_WIN_MENU MenuBarItems;
    PYORI_WIN_CTRL_HANDLE Ctrl;
    YORI_ALLOC_SIZE_T MenuIndex;

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

    YoriLibConstantString(&FileMenuEntries[MenuIndex].Caption, _T("Save As Dev&ice..."));
    FileMenuEntries[MenuIndex].NotifyCallback = HexEditSaveAsDeviceButtonClicked;
    MenuIndex++;

    FileMenuEntries[MenuIndex].Flags = YORI_WIN_MENU_ENTRY_SEPERATOR;
    MenuIndex++;

    YoriLibConstantString(&FileMenuEntries[MenuIndex].Caption, _T("E&xit"));
    YoriLibConstantString(&FileMenuEntries[MenuIndex].Hotkey, _T("Ctrl+Q"));
    FileMenuEntries[MenuIndex].NotifyCallback = HexEditExitButtonClicked;
    MenuIndex++;

    MenuIndex = 0;
    ZeroMemory(&EditMenuEntries, sizeof(EditMenuEntries));

    HexEditContext->EditCutMenuIndex = MenuIndex;
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Caption, _T("Cu&t"));
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Hotkey, _T("Ctrl+X"));
    EditMenuEntries[MenuIndex].NotifyCallback = HexEditCutButtonClicked;
    MenuIndex++;

    HexEditContext->EditCopyMenuIndex = MenuIndex;
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Caption, _T("&Copy"));
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Hotkey, _T("Ctrl+C"));
    EditMenuEntries[MenuIndex].NotifyCallback = HexEditCopyButtonClicked;
    MenuIndex++;

    HexEditContext->EditPasteMenuIndex = MenuIndex;
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Caption, _T("&Paste"));
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Hotkey, _T("Ctrl+V"));
    EditMenuEntries[MenuIndex].NotifyCallback = HexEditPasteButtonClicked;
    MenuIndex++;

    HexEditContext->EditClearMenuIndex = MenuIndex;
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Caption, _T("Cl&ear"));
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Hotkey, _T("Del"));
    EditMenuEntries[MenuIndex].NotifyCallback = HexEditClearButtonClicked;
    MenuIndex++;

    ZeroMemory(&SearchMenuEntries, sizeof(SearchMenuEntries));
    MenuIndex = 0;

    YoriLibConstantString(&SearchMenuEntries[MenuIndex].Caption, _T("&Find..."));
    YoriLibConstantString(&SearchMenuEntries[MenuIndex].Hotkey, _T("Ctrl+F"));
    SearchMenuEntries[MenuIndex].NotifyCallback = HexEditFindButtonClicked;
    MenuIndex++;

    YoriLibConstantString(&SearchMenuEntries[MenuIndex].Caption, _T("&Repeat Last Find"));
    YoriLibConstantString(&SearchMenuEntries[MenuIndex].Hotkey, _T("F3"));
    SearchMenuEntries[MenuIndex].NotifyCallback = HexEditFindNextButtonClicked;
    MenuIndex++;

    YoriLibConstantString(&SearchMenuEntries[MenuIndex].Caption, _T("Find &Previous"));
    YoriLibConstantString(&SearchMenuEntries[MenuIndex].Hotkey, _T("Shift+F3"));
    SearchMenuEntries[MenuIndex].NotifyCallback = HexEditFindPreviousButtonClicked;
    MenuIndex++;

    YoriLibConstantString(&SearchMenuEntries[MenuIndex].Caption, _T("&Change..."));
    YoriLibConstantString(&SearchMenuEntries[MenuIndex].Hotkey, _T("Ctrl+R"));
    SearchMenuEntries[MenuIndex].NotifyCallback = HexEditChangeButtonClicked;
    MenuIndex++;

    SearchMenuEntries[MenuIndex].Flags = YORI_WIN_MENU_ENTRY_SEPERATOR;
    MenuIndex++;

    YoriLibConstantString(&SearchMenuEntries[MenuIndex].Caption, _T("&Go to..."));
    YoriLibConstantString(&SearchMenuEntries[MenuIndex].Hotkey, _T("Ctrl+G"));
    SearchMenuEntries[MenuIndex].NotifyCallback = HexEditGoToButtonClicked;
    MenuIndex++;

    ZeroMemory(&ViewMenuEntries, sizeof(ViewMenuEntries));
    MenuIndex = 0;
    HexEditContext->ViewBytesMenuIndex = MenuIndex;
    YoriLibConstantString(&ViewMenuEntries[MenuIndex].Caption, _T("&Bytes"));
    YoriLibConstantString(&ViewMenuEntries[MenuIndex].Hotkey, _T("Ctrl+B"));
    ViewMenuEntries[MenuIndex].NotifyCallback = HexEditViewBytesButtonClicked;
    MenuIndex++;

    HexEditContext->ViewWordsMenuIndex = MenuIndex;
    YoriLibConstantString(&ViewMenuEntries[MenuIndex].Caption, _T("&Words"));
    ViewMenuEntries[MenuIndex].NotifyCallback = HexEditViewWordsButtonClicked;
    MenuIndex++;

    HexEditContext->ViewDwordsMenuIndex = MenuIndex;
    YoriLibConstantString(&ViewMenuEntries[MenuIndex].Caption, _T("&DWords"));
    YoriLibConstantString(&ViewMenuEntries[MenuIndex].Hotkey, _T("Ctrl+D"));
    ViewMenuEntries[MenuIndex].NotifyCallback = HexEditViewDwordsButtonClicked;
    MenuIndex++;

    HexEditContext->ViewQwordsMenuIndex = MenuIndex;
    YoriLibConstantString(&ViewMenuEntries[MenuIndex].Caption, _T("&QWords"));
    ViewMenuEntries[MenuIndex].NotifyCallback = HexEditViewQwordsButtonClicked;
    MenuIndex++;

    ViewMenuEntries[MenuIndex].Flags = YORI_WIN_MENU_ENTRY_SEPERATOR;
    MenuIndex++;

    HexEditContext->ViewNoOffsetMenuIndex = MenuIndex;
    YoriLibConstantString(&ViewMenuEntries[MenuIndex].Caption, _T("&No Offset"));
    ViewMenuEntries[MenuIndex].NotifyCallback = HexEditViewNoOffsetButtonClicked;
    MenuIndex++;

    HexEditContext->ViewShortOffsetMenuIndex = MenuIndex;
    YoriLibConstantString(&ViewMenuEntries[MenuIndex].Caption, _T("&Short offset"));
    ViewMenuEntries[MenuIndex].NotifyCallback = HexEditViewShortOffsetButtonClicked;
    MenuIndex++;

    HexEditContext->ViewLongOffsetMenuIndex = MenuIndex;
    YoriLibConstantString(&ViewMenuEntries[MenuIndex].Caption, _T("&Long offset"));
    ViewMenuEntries[MenuIndex].NotifyCallback = HexEditViewLongOffsetButtonClicked;
    MenuIndex++;

    ZeroMemory(&ToolsMenuEntries, sizeof(ToolsMenuEntries));
    MenuIndex = 0;
    YoriLibConstantString(&ToolsMenuEntries[MenuIndex].Caption, _T("&Calculate PE Checksum"));
    ToolsMenuEntries[MenuIndex].NotifyCallback = HexEditCalculatePEChecksumButtonClicked;

    ZeroMemory(&HelpMenuEntries, sizeof(HelpMenuEntries));
    MenuIndex = 0;
    YoriLibConstantString(&HelpMenuEntries[MenuIndex].Caption, _T("&About..."));
    HelpMenuEntries[MenuIndex].NotifyCallback = HexEditAboutButtonClicked;

    MenuBarItems.ItemCount = sizeof(MenuEntries)/sizeof(MenuEntries[0]);
    MenuBarItems.Items = MenuEntries;

    MenuIndex = 0;
    ZeroMemory(MenuEntries, sizeof(MenuEntries));
    YoriLibConstantString(&MenuEntries[MenuIndex].Caption, _T("&File"));
    MenuEntries[MenuIndex].ChildMenu.ItemCount = sizeof(FileMenuEntries)/sizeof(FileMenuEntries[0]);
    MenuEntries[MenuIndex].ChildMenu.Items = FileMenuEntries;
    MenuIndex++;

    HexEditContext->EditMenuIndex = MenuIndex;
    YoriLibConstantString(&MenuEntries[MenuIndex].Caption, _T("&Edit"));
    MenuEntries[MenuIndex].NotifyCallback = HexEditEditButtonClicked;
    MenuEntries[MenuIndex].ChildMenu.ItemCount = sizeof(EditMenuEntries)/sizeof(EditMenuEntries[0]);
    MenuEntries[MenuIndex].ChildMenu.Items = EditMenuEntries;
    MenuIndex++;

    YoriLibConstantString(&MenuEntries[MenuIndex].Caption, _T("&Search"));
    MenuEntries[MenuIndex].ChildMenu.ItemCount = sizeof(SearchMenuEntries)/sizeof(SearchMenuEntries[0]);
    MenuEntries[MenuIndex].ChildMenu.Items = SearchMenuEntries;
    MenuIndex++;

    HexEditContext->ViewMenuIndex = MenuIndex;
    YoriLibConstantString(&MenuEntries[MenuIndex].Caption, _T("&View"));
    MenuEntries[MenuIndex].NotifyCallback = HexEditViewButtonClicked;
    MenuEntries[MenuIndex].ChildMenu.ItemCount = sizeof(ViewMenuEntries)/sizeof(ViewMenuEntries[0]);
    MenuEntries[MenuIndex].ChildMenu.Items = ViewMenuEntries;
    MenuIndex++;

    YoriLibConstantString(&MenuEntries[MenuIndex].Caption, _T("&Tools"));
    MenuEntries[MenuIndex].ChildMenu.ItemCount = sizeof(ToolsMenuEntries)/sizeof(ToolsMenuEntries[0]);
    MenuEntries[MenuIndex].ChildMenu.Items = ToolsMenuEntries;
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
    DWORD Style;

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

    if (HexEditContext->OffsetWidth == (UCHAR)-1) {
        if (WindowSize.X < 77) {
            HexEditContext->OffsetWidth = 0;
        } else if (WindowSize.X < 86) {
            HexEditContext->OffsetWidth = 32;
        } else {
            HexEditContext->OffsetWidth = 64;
        }
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

    Style = 0;
    if (HexEditContext->OffsetWidth == 64) {
        Style = YORI_WIN_HEX_EDIT_STYLE_LARGE_OFFSET;
    } else if (HexEditContext->OffsetWidth == 32) {
        Style = YORI_WIN_HEX_EDIT_STYLE_OFFSET;
    }
    HexEdit = YoriWinHexEditCreate(Parent, NULL, &Rect, HexEditContext->BytesPerWord, YORI_WIN_HEX_EDIT_STYLE_VSCROLLBAR | Style);
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
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOLEAN ArgumentUnderstood;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 0;
    DWORD Result;
    YORI_STRING Arg;

    ZeroMemory(&GlobalHexEditContext, sizeof(GlobalHexEditContext));
    GlobalHexEditContext.OffsetWidth = (UCHAR)-1;
    GlobalHexEditContext.BytesPerWord = 1;

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
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                GlobalHexEditContext.BytesPerWord = 1;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("d")) == 0) {
                GlobalHexEditContext.BytesPerWord = 4;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("l")) == 0) {
                GlobalHexEditContext.OffsetWidth = 64;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("n")) == 0) {
                GlobalHexEditContext.OffsetWidth = 0;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("q")) == 0) {
                GlobalHexEditContext.BytesPerWord = 8;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("r")) == 0) {
                GlobalHexEditContext.ReadOnly = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                GlobalHexEditContext.OffsetWidth = 32;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("w")) == 0) {
                GlobalHexEditContext.BytesPerWord = 2;
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
