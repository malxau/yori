/**
 * @file edit/edit.c
 *
 * Yori shell text editor
 *
 * Copyright (c) 2020 Malcolm J. Smith
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
#include "edit.h"

/**
 Help text to display to the user.
 */
const
CHAR strEditHelpText[] =
        "\n"
        "Displays editor.\n"
        "\n"
        "EDIT [-license] [-a] [-e encoding] [-m]\n"
        "\n"
        "   -a             Use ASCII characters for drawing\n"
        "   -e <encoding>  Specifies the character encoding to use\n"
        "   -m             Use modern keyboard navigation instead of Edit compatible\n";

/**
 The copyright year string to display with license text.
 */
const
TCHAR strCopyrightYear[] = _T("2020");

/**
 Display usage text to the user.
 */
BOOL
EditHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Edit %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strEditHelpText);
    return TRUE;
}

/**
 A context that records files found and being operated on in the current
 window.
 */
typedef struct _EDIT_CONTEXT {

    /**
     Pointer to the multiline edit control.
     */
    PYORI_WIN_CTRL_HANDLE MultilineEdit;

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
     The string that was most recently searched for.
     */
    YORI_STRING SearchString;

    /**
     The newline string to use.
     */
    YORI_STRING Newline;

    /**
     The character encoding to use.
     */
    DWORD Encoding;

    /**
     The index of the edit menu.  This is used to enable and disable menu
     items based on the state of the control and clipboard.
     */
    DWORD EditMenuIndex;

    /**
     The index of the undo menu item.
     */
    DWORD EditUndoMenuIndex;

    /**
     The index of the redo menu item.
     */
    DWORD EditRedoMenuIndex;

    /**
     The index of the cut menu item.
     */
    DWORD EditCutMenuIndex;

    /**
     The index of the copy menu item.
     */
    DWORD EditCopyMenuIndex;

    /**
     The index of the paste menu item.
     */
    DWORD EditPasteMenuIndex;

    /**
     The index of the clear menu item.
     */
    DWORD EditClearMenuIndex;

    /**
     The index of the options menu.
     */
    DWORD OptionsMenuIndex;

    /**
     The index of the traditional item in the options menu.
     */
    DWORD OptionsTraditionalMenuIndex;

    /**
     TRUE if the search should be case sensitive.  FALSE if it should be case
     insensitive.
     */
    BOOLEAN SearchMatchCase;

    /**
     TRUE to enable traditional MS-DOS edit navigation, where the cursor can
     move infinitely right on any line.  FALSE to use more modern Windows
     style multiline semantics, where left at the start of the line moves to
     the previous line, or right at the end of a line moves to the next line.
     */
    BOOLEAN TraditionalNavigation;

    /**
     TRUE to use only 7 bit ASCII characters for visual display.
     */
    BOOLEAN UseAsciiDrawing;

} EDIT_CONTEXT, *PEDIT_CONTEXT;

/**
 Global context for the edit application.
 */
EDIT_CONTEXT GlobalEditContext;

/**
 Free all found files in the list.

 @param EditContext Pointer to the edit context.
 */
VOID
EditFreeEditContext(
    __in PEDIT_CONTEXT EditContext
    )
{
    YoriLibFreeStringContents(&EditContext->OpenFileName);
    YoriLibFreeStringContents(&EditContext->SearchString);
}

/**
 Set the caption on the edit control to match the file name component of the
 currently opened file.
 */
VOID
EditUpdateOpenedFileCaption(
    __in PEDIT_CONTEXT EditContext
    )
{
    YORI_STRING NewCaption;
    LPTSTR FinalSlash;

    YoriLibInitEmptyString(&NewCaption);

    FinalSlash = YoriLibFindRightMostCharacter(&EditContext->OpenFileName, '\\');
    if (FinalSlash != NULL) {
        NewCaption.StartOfString = FinalSlash + 1;
        NewCaption.LengthInChars = (DWORD)(EditContext->OpenFileName.LengthInChars - (FinalSlash - EditContext->OpenFileName.StartOfString + 1));
    } else {
        NewCaption.StartOfString = EditContext->OpenFileName.StartOfString;
        NewCaption.LengthInChars = EditContext->OpenFileName.LengthInChars;
    }

    YoriWinMultilineEditSetCaption(EditContext->MultilineEdit, &NewCaption);
}

/**
 Process a single opened stream, enumerating through all lines and populating
 the multiline edit control with the contents.

 @param EditContext Pointer to the edit context.

 @param hSource The opened source stream.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
EditPopulateFromStream(
    __in PEDIT_CONTEXT EditContext,
    __in HANDLE hSource
    )
{
    PVOID LineContext = NULL;
    YORI_STRING LineString;
    PTCHAR NewLine;
    DWORD BytesRequired;
    DWORD BytesAfterAlignment;
    PUCHAR Buffer = NULL;
    DWORD BytesRemainingInBuffer = 0;
    DWORD BufferOffset = 0;
    DWORD Alignment;
    BOOLEAN Result;
    DWORD LinesAllocated;
    DWORD LinesPopulated;
    PYORI_STRING LineArray;
    YORI_LIB_LINE_ENDING FirstLineEnding;
    YORI_LIB_LINE_ENDING LineEnding;
    BOOL TimeoutReached;

    LineArray = NULL;
    LinesAllocated = 0;
    LinesPopulated = 0;

    FirstLineEnding = YoriLibLineEndingNone;

    YoriLibInitEmptyString(&LineString);
    Result = TRUE;

    while (TRUE) {

        if (!YoriLibReadLineToStringEx(&LineString, &LineContext, TRUE, INFINITE, hSource, &LineEnding, &TimeoutReached)) {
            break;
        }

        if (FirstLineEnding == YoriLibLineEndingNone && LineEnding != YoriLibLineEndingNone) {
            FirstLineEnding = LineEnding;
        }

        BytesRequired = (LineString.LengthInChars + 1) * sizeof(TCHAR);

        //
        //  Align to 8 bytes for the next line.
        //

        Alignment = (BufferOffset + BytesRequired) % 8;
        if (Alignment > 0) {
            Alignment = 8 - Alignment;
        }
        BytesAfterAlignment = BytesRequired + Alignment;

        //
        //  If we need a buffer, allocate a buffer that typically has space for
        //  multiple lines
        //

        if (Buffer == NULL || BytesAfterAlignment > BytesRemainingInBuffer) {
            if (Buffer != NULL) {
                YoriLibDereference(Buffer);
            }
            BytesRemainingInBuffer = 64 * 1024;
            if (BytesAfterAlignment > BytesRemainingInBuffer) {
                BytesRemainingInBuffer = BytesAfterAlignment;
            }
            BufferOffset = 0;

            Buffer = YoriLibReferencedMalloc(BytesRemainingInBuffer);
            if (Buffer == NULL) {
                Result = FALSE;
                break;
            }
        }

        //
        //  See if more lines in the line array need to be allocated
        //

        if (LinesPopulated == LinesAllocated) {
            PYORI_STRING NewLineArray;
            DWORD NewLineCount;

            NewLineCount = LinesAllocated * 2;
            if (NewLineCount < 0x1000) {
                NewLineCount = 0x1000;
            }

            NewLineArray = YoriLibReferencedMalloc(NewLineCount * sizeof(YORI_STRING));
            if (NewLineArray == NULL) {
                Result = FALSE;
                break;
            }

            if (LinesPopulated > 0) {
                memcpy(NewLineArray, LineArray, LinesPopulated * sizeof(YORI_STRING));
            }

            if (LineArray != NULL) {
                YoriLibDereference(LineArray);
            }
            LineArray = NewLineArray;
            LinesAllocated = NewLineCount;
        }

        //
        //  Write this line into the current buffer
        //

        NewLine = (PTCHAR)YoriLibAddToPointer(Buffer, BufferOffset);
        YoriLibReference(Buffer);

        LineArray[LinesPopulated].MemoryToFree = Buffer;
        LineArray[LinesPopulated].StartOfString = NewLine;
        LineArray[LinesPopulated].LengthAllocated = BytesRequired / sizeof(TCHAR);
        LineArray[LinesPopulated].LengthInChars = LineArray[LinesPopulated].LengthAllocated - 1;

        memcpy(NewLine, LineString.StartOfString, LineString.LengthInChars * sizeof(TCHAR));
        NewLine[LineString.LengthInChars] = '\0';

        LinesPopulated++;

        //
        //  Align to 8 bytes for the next line.
        //

        BufferOffset += BytesAfterAlignment;
        BytesRemainingInBuffer -= BytesAfterAlignment;
    }

    if (Buffer != NULL) {
        YoriLibDereference(Buffer);
    }

    YoriLibLineReadCloseOrCache(LineContext);
    YoriLibFreeStringContents(&LineString);

    if (Result == FALSE) {
        while (LinesPopulated > 0) {
            YoriLibFreeStringContents(&LineArray[LinesPopulated - 1]);
            LinesPopulated--;
        }

        if (LineArray != NULL) {
            YoriLibDereference(LineArray);
        }

        return FALSE;
    }

    if (LinesPopulated > 0) {
        YoriWinMultilineEditAppendLinesNoDataCopy(EditContext->MultilineEdit, LineArray, LinesPopulated);

        YoriLibConstantString(&EditContext->Newline, _T("\r\n"));
        if (FirstLineEnding == YoriLibLineEndingLF) {
            YoriLibConstantString(&EditContext->Newline, _T("\n"));
        } else if (FirstLineEnding == YoriLibLineEndingCR) {
            YoriLibConstantString(&EditContext->Newline, _T("\r"));
        }
    }

    if (LineArray != NULL) {
        YoriLibDereference(LineArray);
    }

    return Result;
}

/**
 Load the contents of the specified file into the edit window.

 @param EditContext Pointer to the edit context.

 @param FileName Pointer to the name of the file to open.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
EditLoadFile(
    __in PEDIT_CONTEXT EditContext,
    __in PYORI_STRING FileName
    )
{
    HANDLE hFile;
    DWORD SavedEncoding;

    if (FileName->StartOfString == NULL) {
        return FALSE;
    }

    ASSERT(YoriLibIsStringNullTerminated(FileName));

    hFile = CreateFile(FileName->StartOfString, FILE_READ_DATA | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    if (EditContext->Encoding == CP_UTF8_OR_16) {
        DWORD NewEncoding;
        DWORD BytesRead;
        UCHAR LeadingBytes[2];

        NewEncoding = CP_UTF8;

        if (ReadFile(hFile, LeadingBytes, sizeof(LeadingBytes), &BytesRead, NULL)) {
            if (BytesRead == sizeof(LeadingBytes) &&
                LeadingBytes[0] == 0xFF &&
                LeadingBytes[1] == 0xFE) {

                NewEncoding = CP_UTF16;
            }
        }

        SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
        EditContext->Encoding = NewEncoding;
    }

    YoriWinMultilineEditClear(EditContext->MultilineEdit);
    SavedEncoding = YoriLibGetMultibyteInputEncoding();
    YoriLibSetMultibyteInputEncoding(EditContext->Encoding);
    EditPopulateFromStream(EditContext, hFile);
    YoriLibSetMultibyteInputEncoding(SavedEncoding);
    CloseHandle(hFile);
    return TRUE;
}

/**
 Save the contents of the opened window into a file.

 @param EditContext Pointer to the edit context.

 @param FileName Pointer to the name of the file to save.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
EditSaveFile(
    __in PEDIT_CONTEXT EditContext,
    __in PYORI_STRING FileName
    )
{
    DWORD LineIndex;
    DWORD LineCount;
    DWORD SavedEncoding;
    DWORD Index;
    PYORI_STRING Line;
    YORI_STRING ParentDirectory;
    YORI_STRING Prefix;
    YORI_STRING TempFileName;
    HANDLE TempHandle;

    if (FileName->StartOfString == NULL) {
        return FALSE;
    }

    if (EditContext->Newline.StartOfString == NULL) {
        YoriLibConstantString(&EditContext->Newline, _T("\r\n"));
        __analysis_assume(EditContext->Newline.StartOfString != NULL);
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

    //
    //  Write all of the lines to the temporary file and abort on failure.
    //

    LineCount = YoriWinMultilineEditGetLineCount(EditContext->MultilineEdit);

    if (EditContext->Encoding == CP_UTF8_OR_16) {
        EditContext->Encoding = CP_UTF8;
    }

    SavedEncoding = YoriLibGetMultibyteOutputEncoding();
    YoriLibSetMultibyteOutputEncoding(EditContext->Encoding);
    for (LineIndex = 0; LineIndex < LineCount; LineIndex++) {
        Line = YoriWinMultilineEditGetLineByIndex(EditContext->MultilineEdit, LineIndex);
        if (Line->LengthInChars > 0) {
            if (!YoriLibOutputTextToMultibyteDevice(TempHandle, Line)) {
                CloseHandle(TempHandle);
                DeleteFile(TempFileName.StartOfString);
                YoriLibFreeStringContents(&TempFileName);
                return FALSE;
            }
        }
        if (!YoriLibOutputTextToMultibyteDevice(TempHandle, &EditContext->Newline)) {
            CloseHandle(TempHandle);
            DeleteFile(TempFileName.StartOfString);
            YoriLibFreeStringContents(&TempFileName);
            return FALSE;
        }
    }
    YoriLibSetMultibyteOutputEncoding(SavedEncoding);

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
    if (!MoveFileEx(TempFileName.StartOfString, FileName->StartOfString, MOVEFILE_REPLACE_EXISTING)) {
        DeleteFile(TempFileName.StartOfString);
        YoriLibFreeStringContents(&TempFileName);
        return FALSE;
    }

    YoriLibFreeStringContents(&TempFileName);
    return TRUE;
}

VOID
EditSaveButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    );

VOID
EditSaveAsButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    );

/**
 If the file has been modified, prompt the user to save it, and save it if
 requested.

 @param Ctrl Pointer to the menu control indicating the action that triggered
        this prompt.

 @param EditContext Pointer to the edit context.

 @return TRUE to indicate that the requested action should proceed, FALSE to
         indicate the user has cancelled the request.
 */
BOOLEAN
EditPromptForSaveIfModified(
    __in PYORI_WIN_CTRL_HANDLE Ctrl,
    __in PEDIT_CONTEXT EditContext
    )
{
    if (YoriWinMultilineEditGetModifyState(EditContext->MultilineEdit)) {
        PYORI_WIN_CTRL_HANDLE Parent;
        YORI_STRING Title;
        YORI_STRING Text;
        YORI_STRING ButtonText[3];
        DWORD ButtonId;

        Parent = YoriWinGetControlParent(EditContext->MultilineEdit);

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
            if (EditContext->OpenFileName.LengthInChars > 0) {
                EditSaveButtonClicked(Ctrl);
            } else {
                EditSaveAsButtonClicked(Ctrl);
            }

            //
            //  If the buffer is still modified, that implies the save didn't
            //  happen, so cancel.
            //

            if (YoriWinMultilineEditGetModifyState(EditContext->MultilineEdit)) {
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
EditNewButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PEDIT_CONTEXT EditContext;
    PYORI_WIN_CTRL_HANDLE Parent;

    Parent = YoriWinGetControlParent(Ctrl);
    EditContext = YoriWinGetControlContext(Parent);

    if (!EditPromptForSaveIfModified(Ctrl, EditContext)) {
        return;
    }

    YoriWinMultilineEditClear(EditContext->MultilineEdit);
    YoriLibFreeStringContents(&EditContext->OpenFileName);
    EditUpdateOpenedFileCaption(EditContext);
    YoriWinMultilineEditSetModifyState(EditContext->MultilineEdit, FALSE);
}

/**
 Populate the combo box values for encodings for the open and save as
 dialogs.  UTF-8 is the default wherever possible but is not available
 on NT 3.x.

 @param EncodingValues Pointer to an array of values to populate.  The array
        must have space for at least four elements.

 @param EncodingsForOpen If TRUE, this is for an open operation, so encodings
        can include those that involve detection.  If FALSE, this is for a
        save operation, and cannot be heuristically detected.

 @return The number of elements populated into the array.
 */
DWORD
EditPopulateEncodingArray(
    __out_ecount(5) PYORI_DLG_FILE_CUSTOM_VALUE EncodingValues,
    __in BOOLEAN EncodingsForOpen
    )
{
    DWORD EncodingIndex;

    EncodingIndex = 0;
    if (YoriLibIsUtf8Supported()) {
        if (EncodingsForOpen) {
            YoriLibConstantString(&EncodingValues[EncodingIndex++].ValueText, _T("UTF-8/16 based on BOM"));
        }
        YoriLibConstantString(&EncodingValues[EncodingIndex++].ValueText, _T("UTF-8"));
    }
    YoriLibConstantString(&EncodingValues[EncodingIndex++].ValueText, _T("ANSI"));
    YoriLibConstantString(&EncodingValues[EncodingIndex++].ValueText, _T("ASCII"));
    YoriLibConstantString(&EncodingValues[EncodingIndex++].ValueText, _T("UTF-16"));

    return EncodingIndex;
}

/**
 Determine the encoding to use given the array index of the selected combo
 box item in an open or save as dialog.

 @param EncodingIndex The zero based array index of the encoding that the
        user selected.

 @param EncodingsForOpen If TRUE, this is for an open operation, so encodings
        can include those that involve detection.  If FALSE, this is for a
        save operation, and cannot be heuristically detected.

 @return the CP_ encoding value to use.  Can return -1 on failure, but this
         implies the supplied index is not valid.
 */
DWORD
EditEncodingFromArrayIndex(
    __in DWORD EncodingIndex,
    __in BOOLEAN EncodingsForOpen
    )
{
    DWORD Index;
    DWORD Encoding;

    Index = EncodingIndex;
    if (YoriLibIsUtf8Supported()) {
        if (EncodingsForOpen) {
            if (Index == 0) {
                return CP_UTF8_OR_16;
            } else if (Index == 1) {
                return CP_UTF8;
            }
            Index--;
        } else {
            if (Index == 0) {
                return CP_UTF8;
            }
        }
        Index--;
    }

    Encoding = (DWORD)-1;

    switch(Index) {
        case 0:
            Encoding = CP_ACP;
            break;
        case 1:
            Encoding = CP_OEMCP;
            break;
        case 2:
            Encoding = CP_UTF16;
            break;
    }

    return Encoding;
}

/**
 A callback invoked when the open menu item is invoked.

 @param Ctrl Pointer to the menu bar control.
 */
VOID
EditOpenButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    YORI_STRING Title;
    YORI_STRING Text;
    YORI_STRING FullName;
    PEDIT_CONTEXT EditContext;
    YORI_DLG_FILE_CUSTOM_VALUE EncodingValues[5];
    YORI_DLG_FILE_CUSTOM_OPTION CustomOptionArray[1];
    DWORD Encoding;
    DWORD EncodingCount;

    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    EditContext = YoriWinGetControlContext(Parent);

    EncodingCount = EditPopulateEncodingArray(EncodingValues, TRUE);

    YoriLibConstantString(&CustomOptionArray[0].Description, _T("&Encoding:"));
    CustomOptionArray[0].ValueCount = EncodingCount;
    CustomOptionArray[0].Values = EncodingValues;
    CustomOptionArray[0].SelectedValue = 0;

    YoriLibConstantString(&Title, _T("Open"));
    YoriLibInitEmptyString(&Text);

    YoriDlgFile(YoriWinGetWindowManagerHandle(Parent),
                &Title,
                sizeof(CustomOptionArray)/sizeof(CustomOptionArray[0]),
                CustomOptionArray,
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

    Encoding = EditEncodingFromArrayIndex(CustomOptionArray[0].SelectedValue, TRUE);
    if (Encoding != -1) {
        EditContext->Encoding = Encoding;
    }

    YoriLibFreeStringContents(&Text);
    if (!EditLoadFile(EditContext, &FullName)) {
        YORI_STRING DialogText;
        YORI_STRING ButtonText;

        YoriLibConstantString(&DialogText, _T("Could not open file"));
        YoriLibConstantString(&ButtonText, _T("Ok"));

        YoriDlgMessageBox(YoriWinGetWindowManagerHandle(Parent),
                          &Title,
                          &DialogText,
                          1,
                          &ButtonText,
                          0,
                          0);

        YoriLibFreeStringContents(&FullName);
        return;
    }

    YoriLibFreeStringContents(&EditContext->OpenFileName);
    memcpy(&EditContext->OpenFileName, &FullName, sizeof(YORI_STRING));
    EditUpdateOpenedFileCaption(EditContext);
    YoriWinMultilineEditSetModifyState(EditContext->MultilineEdit, FALSE);
}

VOID
EditSaveAsButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    );

/**
 A callback invoked when the save menu item is invoked.

 @param Ctrl Pointer to the menu bar control.
 */
VOID
EditSaveButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    YORI_STRING Title;
    YORI_STRING Text;
    YORI_STRING ButtonText;
    PEDIT_CONTEXT EditContext;

    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    EditContext = YoriWinGetControlContext(Parent);

    if (EditContext->OpenFileName.StartOfString == NULL) {
        EditSaveAsButtonClicked(Ctrl);
        return;
    }

    YoriLibConstantString(&Title, _T("Save"));
    YoriLibConstantString(&ButtonText, _T("Ok"));

    if (!EditSaveFile(EditContext, &EditContext->OpenFileName)) {
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
    YoriWinMultilineEditSetModifyState(EditContext->MultilineEdit, FALSE);
}

/**
 A callback invoked when the save as menu item is invoked.

 @param Ctrl Pointer to the menu bar control.
 */
VOID
EditSaveAsButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    YORI_STRING Title;
    YORI_STRING Text;
    YORI_STRING FullName;
    PEDIT_CONTEXT EditContext;
    YORI_DLG_FILE_CUSTOM_VALUE EncodingValues[5];
    YORI_DLG_FILE_CUSTOM_VALUE LineEndingValues[3];
    YORI_DLG_FILE_CUSTOM_OPTION CustomOptionArray[2];
    DWORD Encoding;
    DWORD EncodingCount;

    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    EditContext = YoriWinGetControlContext(Parent);

    EncodingCount = EditPopulateEncodingArray(EncodingValues, FALSE);

    YoriLibConstantString(&LineEndingValues[0].ValueText, _T("Windows (CRLF)"));
    YoriLibConstantString(&LineEndingValues[1].ValueText, _T("UNIX (LF)"));
    YoriLibConstantString(&LineEndingValues[2].ValueText, _T("Classic Mac (CR)"));

    YoriLibConstantString(&CustomOptionArray[0].Description, _T("&Encoding:"));
    CustomOptionArray[0].ValueCount = EncodingCount;
    CustomOptionArray[0].Values = EncodingValues;
    CustomOptionArray[0].SelectedValue = 0;

    YoriLibConstantString(&CustomOptionArray[1].Description, _T("&Line ending:"));
    CustomOptionArray[1].ValueCount = sizeof(LineEndingValues)/sizeof(LineEndingValues[1]);
    CustomOptionArray[1].Values = LineEndingValues;
    CustomOptionArray[1].SelectedValue = 0;

    if (YoriLibCompareStringWithLiteral(&EditContext->Newline, _T("\n")) == 0) {
        CustomOptionArray[1].SelectedValue = 1;
    } else if (YoriLibCompareStringWithLiteral(&EditContext->Newline, _T("\r")) == 0) {
        CustomOptionArray[1].SelectedValue = 2;
    }


    YoriLibConstantString(&Title, _T("Save As"));
    YoriLibInitEmptyString(&Text);

    YoriDlgFile(YoriWinGetWindowManagerHandle(Parent),
                &Title,
                sizeof(CustomOptionArray)/sizeof(CustomOptionArray[0]),
                CustomOptionArray,
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

    Encoding = EditEncodingFromArrayIndex(CustomOptionArray[0].SelectedValue, FALSE);

    //
    //  Can't autodetect how to save, only how to open.
    //

    ASSERT(Encoding != CP_UTF8_OR_16);
    if (Encoding != -1) {
        EditContext->Encoding = Encoding;
    }

    switch(CustomOptionArray[1].SelectedValue) {
        case 0:
            YoriLibConstantString(&EditContext->Newline, _T("\r\n"));
            break;
        case 1:
            YoriLibConstantString(&EditContext->Newline, _T("\n"));
            break;
        case 2:
            YoriLibConstantString(&EditContext->Newline, _T("\r"));
            break;
    }

    YoriLibFreeStringContents(&Text);
    if (!EditSaveFile(EditContext, &FullName)) {
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

    YoriLibFreeStringContents(&EditContext->OpenFileName);
    memcpy(&EditContext->OpenFileName, &FullName, sizeof(YORI_STRING));
    EditUpdateOpenedFileCaption(EditContext);
    YoriWinMultilineEditSetModifyState(EditContext->MultilineEdit, FALSE);
}

/**
 A callback invoked when the exit button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
EditExitButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PEDIT_CONTEXT EditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    EditContext = YoriWinGetControlContext(Parent);

    if (!EditPromptForSaveIfModified(Ctrl, EditContext)) {
        return;
    }

    YoriWinCloseWindow(Parent, TRUE);
}

/**
 A callback invoked when the edit menu is opened.

 @param Ctrl Pointer to the menubar control.
 */
VOID
EditEditButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE EditMenu;
    PYORI_WIN_CTRL_HANDLE UndoItem;
    PYORI_WIN_CTRL_HANDLE RedoItem;
    PYORI_WIN_CTRL_HANDLE CutItem;
    PYORI_WIN_CTRL_HANDLE CopyItem;
    PYORI_WIN_CTRL_HANDLE PasteItem;
    PYORI_WIN_CTRL_HANDLE ClearItem;
    BOOLEAN TextSelected;
    YORI_STRING ClipboardText;
    PYORI_WIN_CTRL_HANDLE Parent;
    PEDIT_CONTEXT EditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    EditContext = YoriWinGetControlContext(Parent);

    YoriLibInitEmptyString(&ClipboardText);
    YoriLibPasteText(&ClipboardText);

    TextSelected = YoriWinMultilineEditSelectionActive(EditContext->MultilineEdit);
    EditMenu = YoriWinMenuBarGetSubmenuHandle(Ctrl, NULL, EditContext->EditMenuIndex);
    UndoItem = YoriWinMenuBarGetSubmenuHandle(Ctrl, EditMenu, EditContext->EditUndoMenuIndex);
    RedoItem = YoriWinMenuBarGetSubmenuHandle(Ctrl, EditMenu, EditContext->EditRedoMenuIndex);
    CutItem = YoriWinMenuBarGetSubmenuHandle(Ctrl, EditMenu, EditContext->EditCutMenuIndex);
    CopyItem = YoriWinMenuBarGetSubmenuHandle(Ctrl, EditMenu, EditContext->EditCopyMenuIndex);
    PasteItem = YoriWinMenuBarGetSubmenuHandle(Ctrl, EditMenu, EditContext->EditPasteMenuIndex);
    ClearItem = YoriWinMenuBarGetSubmenuHandle(Ctrl, EditMenu, EditContext->EditClearMenuIndex);

    if (YoriWinMultilineEditIsUndoAvailable(EditContext->MultilineEdit)) {
        YoriWinMenuBarEnableMenuItem(UndoItem);
    } else {
        YoriWinMenuBarDisableMenuItem(UndoItem);
    }

    if (YoriWinMultilineEditIsRedoAvailable(EditContext->MultilineEdit)) {
        YoriWinMenuBarEnableMenuItem(RedoItem);
    } else {
        YoriWinMenuBarDisableMenuItem(RedoItem);
    }

    if (TextSelected) {
        YoriWinMenuBarEnableMenuItem(CutItem);
        YoriWinMenuBarEnableMenuItem(CopyItem);
        YoriWinMenuBarEnableMenuItem(ClearItem);
    } else {
        YoriWinMenuBarDisableMenuItem(CutItem);
        YoriWinMenuBarDisableMenuItem(CopyItem);
        YoriWinMenuBarDisableMenuItem(ClearItem);
    }

    if (ClipboardText.LengthInChars > 0) {
        YoriWinMenuBarEnableMenuItem(PasteItem);
    } else {
        YoriWinMenuBarDisableMenuItem(PasteItem);
    }

    YoriLibFreeStringContents(&ClipboardText);
}

/**
 A callback invoked when the undo button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
EditUndoButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PEDIT_CONTEXT EditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    EditContext = YoriWinGetControlContext(Parent);
    YoriWinMultilineEditUndo(EditContext->MultilineEdit);
}

/**
 A callback invoked when the redo button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
EditRedoButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PEDIT_CONTEXT EditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    EditContext = YoriWinGetControlContext(Parent);
    YoriWinMultilineEditRedo(EditContext->MultilineEdit);
}


/**
 A callback invoked when the cut button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
EditCutButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PEDIT_CONTEXT EditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    EditContext = YoriWinGetControlContext(Parent);
    YoriWinMultilineEditCutSelectedText(EditContext->MultilineEdit);
}

/**
 A callback invoked when the copy button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
EditCopyButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PEDIT_CONTEXT EditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    EditContext = YoriWinGetControlContext(Parent);
    YoriWinMultilineEditCopySelectedText(EditContext->MultilineEdit);
}

/**
 A callback invoked when the paste button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
EditPasteButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PEDIT_CONTEXT EditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    EditContext = YoriWinGetControlContext(Parent);
    YoriWinMultilineEditPasteText(EditContext->MultilineEdit);
}

/**
 A callback invoked when the clear button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
EditClearButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PEDIT_CONTEXT EditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    EditContext = YoriWinGetControlContext(Parent);
    YoriWinMultilineEditDeleteSelection(EditContext->MultilineEdit);
}

/**
 Search from a specified point in the multiline edit control to find the
 next matching string.

 @param EditContext Pointer to the edit context specifying the multiline
        edit control, the string to search for, and whether to search case
        sensitively or insensitively.

 @param StartLine The zero based first line to search.

 @param StartOffset The zero based first character in the line to search.

 @param NextMatchLine On successful completion, populated with the zero 
        based line index of the next match.

 @param NextMatchOffset On successful completion, populated with the zero
        based character index of the next match.

 @return TRUE to indicate a match was found, FALSE to indicate it was not.
 */
__success(return)
BOOLEAN
EditFindNextMatchingString(
    __in PEDIT_CONTEXT EditContext,
    __in DWORD StartLine,
    __in DWORD StartOffset,
    __out PDWORD NextMatchLine,
    __out PDWORD NextMatchOffset
    )
{
    DWORD LineCount;
    DWORD LineIndex;
    DWORD Offset;
    YORI_STRING Substring;
    PYORI_STRING Line;
    PYORI_STRING Match;

    if (EditContext->SearchString.LengthInChars == 0) {
        return FALSE;
    }

    LineCount = YoriWinMultilineEditGetLineCount(EditContext->MultilineEdit);
    if (LineCount == 0) {
        return FALSE;
    }

    //
    //  For the line that the cursor is on, extract the substring of text
    //  that follows the cursor and search in that.  If a match is found,
    //  remember to adjust the offset of the selection in the control to
    //  account for the substring offset.
    //

    Line = YoriWinMultilineEditGetLineByIndex(EditContext->MultilineEdit, StartLine);

    if (StartOffset < Line->LengthInChars) {
        YoriLibInitEmptyString(&Substring);
        Substring.StartOfString = Line->StartOfString + StartOffset;
        Substring.LengthInChars = Line->LengthInChars - StartOffset;
        if (EditContext->SearchMatchCase) {
            Match = YoriLibFindFirstMatchingSubstring(&Substring, 1, &EditContext->SearchString, &Offset);
        } else {
            Match = YoriLibFindFirstMatchingSubstringInsensitive(&Substring, 1, &EditContext->SearchString, &Offset);
        }

        if (Match != NULL) {
            *NextMatchLine = StartLine;
            *NextMatchOffset = Offset + StartOffset;
            return TRUE;
        }
    }

    //
    //  Do the rest of the lines the easy way
    //

    for (LineIndex = StartLine + 1; LineIndex < LineCount; LineIndex++) {
        Line = YoriWinMultilineEditGetLineByIndex(EditContext->MultilineEdit, LineIndex);
        if (EditContext->SearchMatchCase) {
            Match = YoriLibFindFirstMatchingSubstring(Line, 1, &EditContext->SearchString, &Offset);
        } else {
            Match = YoriLibFindFirstMatchingSubstringInsensitive(Line, 1, &EditContext->SearchString, &Offset);
        }
        if (Match != NULL) {
            *NextMatchLine = LineIndex;
            *NextMatchOffset = Offset;
            return TRUE;
        }
    }

    return FALSE;
}

/**
 Search from a specified point in the multiline edit control to find the
 previous matching string.

 @param EditContext Pointer to the edit context specifying the multiline
        edit control, the string to search for, and whether to search case
        sensitively or insensitively.

 @param StartLine The zero based first line to search.

 @param StartOffset The zero based first character in the line to search.

 @param NextMatchLine On successful completion, populated with the zero 
        based line index of the next match.

 @param NextMatchOffset On successful completion, populated with the zero
        based character index of the next match.

 @return TRUE to indicate a match was found, FALSE to indicate it was not.
 */
__success(return)
BOOLEAN
EditFindPreviousMatchingString(
    __in PEDIT_CONTEXT EditContext,
    __in DWORD StartLine,
    __in DWORD StartOffset,
    __out PDWORD NextMatchLine,
    __out PDWORD NextMatchOffset
    )
{
    DWORD LineCount;
    DWORD LineIndex;
    DWORD Offset;
    YORI_STRING Substring;
    PYORI_STRING Line;
    PYORI_STRING Match;

    if (EditContext->SearchString.LengthInChars == 0) {
        return FALSE;
    }

    LineCount = YoriWinMultilineEditGetLineCount(EditContext->MultilineEdit);
    if (LineCount == 0) {
        return FALSE;
    }

    //
    //  For the line that the cursor is on, extract the substring of text
    //  that is before the cursor, plus the length of the search string.
    //  This allows for a match at the specified location or anything before
    //  it.
    //

    Line = YoriWinMultilineEditGetLineByIndex(EditContext->MultilineEdit, StartLine);
    YoriLibInitEmptyString(&Substring);
    Substring.StartOfString = Line->StartOfString;
    Substring.LengthInChars = Line->LengthInChars;
    if (StartOffset < Substring.LengthInChars) {
        if (StartOffset + EditContext->SearchString.LengthInChars < Substring.LengthInChars) {
            Substring.LengthInChars = StartOffset + EditContext->SearchString.LengthInChars;
        }
    }
    if (EditContext->SearchMatchCase) {
        Match = YoriLibFindLastMatchingSubstring(&Substring, 1, &EditContext->SearchString, &Offset);
    } else {
        Match = YoriLibFindLastMatchingSubstringInsensitive(&Substring, 1, &EditContext->SearchString, &Offset);
    }

    if (Match != NULL) {
        *NextMatchLine = StartLine;
        *NextMatchOffset = Offset;
        return TRUE;
    }

    //
    //  Do the rest of the lines the easy way
    //

    for (LineIndex = StartLine; LineIndex > 0; LineIndex--) {
        Line = YoriWinMultilineEditGetLineByIndex(EditContext->MultilineEdit, LineIndex - 1);
        if (EditContext->SearchMatchCase) {
            Match = YoriLibFindLastMatchingSubstring(Line, 1, &EditContext->SearchString, &Offset);
        } else {
            Match = YoriLibFindLastMatchingSubstringInsensitive(Line, 1, &EditContext->SearchString, &Offset);
        }
        if (Match != NULL) {
            *NextMatchLine = LineIndex - 1;
            *NextMatchOffset = Offset;
            return TRUE;
        }
    }

    return FALSE;
}

/**
 Find the next match from the cursor position or selection.  This is shared
 between find and find next.

 @param EditContext Pointer to the edit context.

 @return TRUE if a match is found, FALSE if it is not.
 */
BOOLEAN
EditFindNextFromCurrentPosition(
    __in PEDIT_CONTEXT EditContext
    )
{
    DWORD CursorOffset;
    DWORD CursorLine;
    DWORD NextMatchLine;
    DWORD NextMatchOffset;

    DWORD SelectionEndLine;
    DWORD SelectionEndOffset;

    //
    //  If a selection is active, start from the second character in the
    //  selection.  If not, start from the cursor.
    //

    if (!YoriWinMultilineEditGetSelectionRange(EditContext->MultilineEdit, &CursorLine, &CursorOffset, &SelectionEndLine, &SelectionEndOffset)) {
        YoriWinMultilineEditGetCursorLocation(EditContext->MultilineEdit, &CursorOffset, &CursorLine);
    } else {

        //
        //  Move forward for the next match.
        //

        CursorOffset++;
    }

    if (EditFindNextMatchingString(EditContext, CursorLine, CursorOffset, &NextMatchLine, &NextMatchOffset)) {

        YoriWinMultilineEditSetSelectionRange(EditContext->MultilineEdit, NextMatchLine, NextMatchOffset, NextMatchLine, NextMatchOffset + EditContext->SearchString.LengthInChars);
        return TRUE;
    }

    return FALSE;
}

/**
 A callback invoked when the find menu item is invoked.

 @param Ctrl Pointer to the menu bar control.
 */
VOID
EditFindButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    YORI_STRING Title;
    YORI_STRING Text;
    YORI_STRING InitialText;
    BOOLEAN MatchCase;
    PYORI_WIN_CTRL_HANDLE Parent;
    PEDIT_CONTEXT EditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    EditContext = YoriWinGetControlContext(Parent);

    YoriLibConstantString(&Title, _T("Find"));
    YoriLibInitEmptyString(&Text);

    //
    //  Populate the dialog with whatever is selected now, if anything
    //

    YoriLibInitEmptyString(&InitialText);
    if (YoriWinMultilineEditSelectionActive(EditContext->MultilineEdit)) {
        YORI_STRING Newline;

        YoriLibInitEmptyString(&Newline);
        if (!YoriWinMultilineEditGetSelectedText(EditContext->MultilineEdit, &Newline, &InitialText)) {
            YoriLibInitEmptyString(&InitialText);
        }
    }

    if (!YoriDlgFindText(YoriWinGetWindowManagerHandle(Parent),
                         &Title,
                         &InitialText,
                         &MatchCase,
                         &Text)) {

        YoriLibFreeStringContents(&InitialText);
        return;
    }

    YoriLibFreeStringContents(&InitialText);
    if (Text.LengthInChars == 0) {
        YoriLibFreeStringContents(&Text);
        return;
    }

    YoriLibFreeStringContents(&EditContext->SearchString);
    memcpy(&EditContext->SearchString, &Text, sizeof(YORI_STRING));
    EditContext->SearchMatchCase = MatchCase;

    if (!EditFindNextFromCurrentPosition(EditContext)) {
        YORI_STRING ButtonText[1];

        YoriLibConstantString(&Title, _T("Find"));
        YoriLibConstantString(&Text, _T("Text not found."));
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
EditFindNextButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PEDIT_CONTEXT EditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    EditContext = YoriWinGetControlContext(Parent);

    if (EditContext->SearchString.LengthInChars == 0) {
        return;
    }

    if (!EditFindNextFromCurrentPosition(EditContext)) {
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
 A callback invoked when the repeat last find menu item is invoked.

 @param Ctrl Pointer to the menu bar control.
 */
VOID
EditFindPreviousButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    DWORD CursorOffset;
    DWORD CursorLine;
    DWORD NextMatchLine;
    DWORD NextMatchOffset;
    PYORI_WIN_CTRL_HANDLE Parent;
    PEDIT_CONTEXT EditContext;

    DWORD SelectionEndLine;
    DWORD SelectionEndOffset;

    Parent = YoriWinGetControlParent(Ctrl);
    EditContext = YoriWinGetControlContext(Parent);

    if (EditContext->SearchString.LengthInChars == 0) {
        return;
    }

    //
    //  If a selection is active, start from one character before the
    //  beginning of the selection.  If not, start from the cursor.
    //

    if (!YoriWinMultilineEditGetSelectionRange(EditContext->MultilineEdit, &CursorLine, &CursorOffset, &SelectionEndLine, &SelectionEndOffset)) {
        YoriWinMultilineEditGetCursorLocation(EditContext->MultilineEdit, &CursorOffset, &CursorLine);
    } else {

        //
        //  Move back for the next match.
        //

        if (CursorOffset > 0) {
            CursorOffset--;
        } else if (CursorLine > 0) {
            CursorLine--;
            CursorOffset = (DWORD)-1;
        }
    }

    if (EditFindPreviousMatchingString(EditContext, CursorLine, CursorOffset, &NextMatchLine, &NextMatchOffset)) {

        YoriWinMultilineEditSetSelectionRange(EditContext->MultilineEdit, NextMatchLine, NextMatchOffset, NextMatchLine, NextMatchOffset + EditContext->SearchString.LengthInChars);
    } else {
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
EditChangeButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgr;
    YORI_STRING Title;
    YORI_STRING OldText;
    YORI_STRING NewText;
    YORI_STRING InitialBeforeText;
    BOOLEAN MatchCase;
    BOOLEAN ReplaceAll;
    BOOLEAN MatchFound;
    PYORI_WIN_CTRL_HANDLE Parent;
    PEDIT_CONTEXT EditContext;
    DWORD StartLine;
    DWORD StartOffset;
    DWORD NextMatchLine;
    DWORD NextMatchOffset;
    WORD DialogTop;

    Parent = YoriWinGetControlParent(Ctrl);
    EditContext = YoriWinGetControlContext(Parent);
    WinMgr = YoriWinGetWindowManagerHandle(Parent);
    YoriLibInitEmptyString(&OldText);
    YoriLibInitEmptyString(&NewText);
    ReplaceAll = FALSE;
    MatchCase = FALSE;
    MatchFound = FALSE;

    YoriWinMultilineEditGetCursorLocation(EditContext->MultilineEdit, &StartOffset, &StartLine);

    while(TRUE) {

        if (!ReplaceAll) {

            YoriLibConstantString(&Title, _T("Find"));

            //
            //  Populate the dialog with whatever is selected now, if anything
            //

            YoriLibInitEmptyString(&InitialBeforeText);
            if (OldText.LengthInChars > 0) {
                InitialBeforeText.StartOfString = OldText.StartOfString;
                InitialBeforeText.LengthInChars = OldText.LengthInChars;
            } else {
                if (YoriWinMultilineEditSelectionActive(EditContext->MultilineEdit)) {
                    YORI_STRING Newline;

                    YoriLibInitEmptyString(&Newline);
                    if (!YoriWinMultilineEditGetSelectedText(EditContext->MultilineEdit, &Newline, &InitialBeforeText)) {
                        YoriLibInitEmptyString(&InitialBeforeText);
                        InitialBeforeText.StartOfString = OldText.StartOfString;
                        InitialBeforeText.LengthInChars = OldText.LengthInChars;
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
                DWORD CursorLine;
                DWORD CursorOffset;
                DWORD ViewportLeft;
                DWORD ViewportTop;
                WORD DialogHeight;
                WORD RemainingEditHeight;

                if (!YoriWinGetWinMgrDimensions(WinMgr, &WinMgrSize)) {
                    YoriLibFreeStringContents(&InitialBeforeText);
                    break;
                }
                DialogHeight = YoriDlgReplaceGetDialogHeight(WinMgr);
                DialogTop = (SHORT)(WinMgrSize.Y - DialogHeight - 1);

                YoriWinGetControlClientSize(EditContext->MultilineEdit, &ClientSize);
                YoriWinMultilineEditGetCursorLocation(EditContext->MultilineEdit, &CursorOffset, &CursorLine);
                YoriWinMultilineEditGetViewportLocation(EditContext->MultilineEdit, &ViewportLeft, &ViewportTop);

                RemainingEditHeight = (SHORT)(ClientSize.Y - DialogHeight);

                if (CursorLine > (DWORD)(ViewportTop + RemainingEditHeight - 1)) {
                    ViewportTop = CursorLine - (RemainingEditHeight / 2);
                    if (CursorOffset + EditContext->SearchString.LengthInChars < (DWORD)ClientSize.X) {
                        ViewportLeft = 0;
                    }

                    YoriWinMultilineEditSetViewportLocation(EditContext->MultilineEdit, ViewportLeft, ViewportTop);
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

            if (!YoriDlgReplaceText(WinMgr,
                                    (WORD)-1,
                                    DialogTop,
                                    &Title,
                                    &InitialBeforeText,
                                    &NewText,
                                    &MatchCase,
                                    &ReplaceAll,
                                    &OldText,
                                    &NewText)) {

                YoriLibFreeStringContents(&InitialBeforeText);
                break;
            }

            YoriLibFreeStringContents(&InitialBeforeText);
            if (OldText.LengthInChars == 0) {
                YoriLibFreeStringContents(&OldText);
                YoriLibFreeStringContents(&NewText);
                return;
            }

            //
            //  If the search string has changed, move the new one into the
            //  edit context.  Whether it changed or not, OldText still
            //  points to the search string, and MemoryToFree indicates
            //  whether it's owned by the search context or needs to be
            //  freed here.
            //

            if (YoriLibCompareString(&EditContext->SearchString, &OldText) != 0) {
                MatchFound = FALSE;
                YoriLibFreeStringContents(&EditContext->SearchString);
                memcpy(&EditContext->SearchString, &OldText, sizeof(YORI_STRING));
                OldText.MemoryToFree = NULL;
            }

            EditContext->SearchMatchCase = MatchCase;
        }

        if (MatchFound) {
            YoriWinMultilineEditDeleteSelection(EditContext->MultilineEdit);
            YoriWinMultilineEditInsertTextAtCursor(EditContext->MultilineEdit, &NewText);
            StartOffset = StartOffset + NewText.LengthInChars;
        }

        if (!EditFindNextMatchingString(EditContext, StartLine, StartOffset, &NextMatchLine, &NextMatchOffset)) {
            break;
        }

        MatchFound = TRUE;

        //
        //  MSFIX: In the ReplaceAll case, this is still updating the off
        //  screen buffer constantly.  Ideally this wouldn't happen, but
        //  it still needs to be updated once before returning to the user.
        //

        YoriWinMultilineEditSetSelectionRange(EditContext->MultilineEdit, NextMatchLine, NextMatchOffset, NextMatchLine, NextMatchOffset + EditContext->SearchString.LengthInChars);
        StartLine = NextMatchLine;
        StartOffset = NextMatchOffset;
    }

    YoriLibFreeStringContents(&NewText);
    YoriLibFreeStringContents(&OldText);
}

/**
 A callback invoked when the go to line menu item is invoked.

 @param Ctrl Pointer to the menu bar control.
 */
VOID
EditGoToLineButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    YORI_STRING Title;
    YORI_STRING Text;
    PYORI_WIN_CTRL_HANDLE Parent;
    PEDIT_CONTEXT EditContext;
    LONGLONG llNewLine;
    DWORD CharsConsumed;

    Parent = YoriWinGetControlParent(Ctrl);
    EditContext = YoriWinGetControlContext(Parent);

    YoriLibConstantString(&Title, _T("Go to line"));
    YoriLibInitEmptyString(&Text);

    YoriDlgInput(YoriWinGetWindowManagerHandle(Parent),
                 &Title,
                 &Text);

    if (Text.LengthInChars == 0) {
        YoriLibFreeStringContents(&Text);
        return;
    }

    if (YoriLibStringToNumber(&Text, FALSE, &llNewLine, &CharsConsumed) &&
        CharsConsumed > 0) {

        DWORD NewLine;
        NewLine = (DWORD)llNewLine;
        if (NewLine > 0) {
            NewLine--;
        }

        YoriWinMultilineEditSetCursorLocation(EditContext->MultilineEdit, 0, NewLine);
    }

    YoriLibFreeStringContents(&Text);
}

/**
 A callback invoked when the display options menu item is invoked.

 @param Ctrl Pointer to the menu bar control.
 */
VOID
EditDisplayOptionsButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PEDIT_CONTEXT EditContext;
    DWORD NewTabWidth;

    Parent = YoriWinGetControlParent(Ctrl);
    EditContext = YoriWinGetControlContext(Parent);

    if (!YoriWinMultilineEditGetTabWidth(EditContext->MultilineEdit, &NewTabWidth)) {
        return;
    }

    if (!EditOpts(YoriWinGetWindowManagerHandle(Parent), NewTabWidth, &NewTabWidth)) {
        return;
    }

    YoriWinMultilineEditSetTabWidth(EditContext->MultilineEdit, NewTabWidth);
}

/**
 A callback invoked when the traditional navigation options menu item is
 invoked.

 @param Ctrl Pointer to the menu bar control.
 */
VOID
EditTraditionalNavigationOptionsButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PYORI_WIN_CTRL_HANDLE OptionsMenu;
    PYORI_WIN_CTRL_HANDLE TraditionalNavigationMenuItem;

    PEDIT_CONTEXT EditContext;
    Parent = YoriWinGetControlParent(Ctrl);
    EditContext = YoriWinGetControlContext(Parent);

    OptionsMenu = YoriWinMenuBarGetSubmenuHandle(Ctrl, NULL, EditContext->OptionsMenuIndex);
    TraditionalNavigationMenuItem = YoriWinMenuBarGetSubmenuHandle(Ctrl, OptionsMenu, EditContext->OptionsTraditionalMenuIndex);


    if (EditContext->TraditionalNavigation) {
        EditContext->TraditionalNavigation = FALSE;
        YoriWinMenuBarUncheckMenuItem(TraditionalNavigationMenuItem);
    } else {
        EditContext->TraditionalNavigation = TRUE;
        YoriWinMenuBarCheckMenuItem(TraditionalNavigationMenuItem);
    }
    YoriWinMultilineEditSetTraditionalNavigation(EditContext->MultilineEdit, EditContext->TraditionalNavigation);
}


/**
 A callback invoked when the about menu item is invoked.

 @param Ctrl Pointer to the menu bar control.
 */
VOID
EditAboutButtonClicked(
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
                   _T("Edit %i.%02i\n")
#if YORI_BUILD_ID
                   _T("  Build %i\n")
#endif
                   _T("%hs"),

                   YORI_VER_MAJOR,
                   YORI_VER_MINOR,
#if YORI_BUILD_ID
                   YORI_BUILD_ID,
#endif
                   strEditHelpText);

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

    ButtonClicked = EditAboutDialog(YoriWinGetWindowManagerHandle(Parent),
                                    &Title,
                                    &CenteredText,
                                    &LeftText,
                                    2,
                                    ButtonTexts,
                                    0,
                                    0);

    YoriLibFreeStringContents(&Text);

    if (ButtonClicked == 2) {
        if (YoriLibMitLicenseText(strCopyrightYear, &Text)) {

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

            EditAboutDialog(YoriWinGetWindowManagerHandle(Parent),
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
 A callback from the multiline edit control to indicate the cursor has moved
 and the status bar should be updated.

 @param Ctrl Pointer to the multiline edit control.

 @param CursorOffset The horizontal offset of the cursor in buffer
        coordinates.

 @param CursorLine The vertical offset of the cursor in buffer coordinates.
 */
VOID
EditNotifyCursorMove(
    __in PYORI_WIN_CTRL_HANDLE Ctrl,
    __in DWORD CursorOffset,
    __in DWORD CursorLine
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PEDIT_CONTEXT EditContext;
    YORI_STRING NewStatus;

    Parent = YoriWinGetControlParent(Ctrl);
    EditContext = YoriWinGetControlContext(Parent);

    YoriLibInitEmptyString(&NewStatus);
    YoriLibYPrintf(&NewStatus, _T("%06i:%04i "), CursorLine + 1, CursorOffset + 1);

    YoriWinLabelSetCaption(EditContext->StatusBar, &NewStatus);
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

 @param EditContext Pointer to the edit context.

 @param Parent Handle to the main window.

 @return Pointer to the menu bar control if it was successfully created
         and populated, or NULL on failure.
 */
PYORI_WIN_CTRL_HANDLE
EditPopulateMenuBar(
    __in PEDIT_CONTEXT EditContext,
    __in PYORI_WIN_WINDOW_HANDLE Parent
    )
{
    YORI_WIN_MENU_ENTRY FileMenuEntries[6];
    YORI_WIN_MENU_ENTRY EditMenuEntries[7];
    YORI_WIN_MENU_ENTRY SearchMenuEntries[6];
    YORI_WIN_MENU_ENTRY OptionsMenuEntries[2];
    YORI_WIN_MENU_ENTRY HelpMenuEntries[1];
    YORI_WIN_MENU_ENTRY MenuEntries[5];
    YORI_WIN_MENU MenuBarItems;
    PYORI_WIN_CTRL_HANDLE Ctrl;
    DWORD MenuIndex;

    ZeroMemory(&FileMenuEntries, sizeof(FileMenuEntries));
    MenuIndex = 0;
    YoriLibConstantString(&FileMenuEntries[MenuIndex].Caption, _T("&New"));
    YoriLibConstantString(&FileMenuEntries[MenuIndex].Hotkey, _T("Ctrl+N"));
    FileMenuEntries[MenuIndex].NotifyCallback = EditNewButtonClicked;

    MenuIndex++;
    YoriLibConstantString(&FileMenuEntries[MenuIndex].Caption, _T("&Open..."));
    YoriLibConstantString(&FileMenuEntries[MenuIndex].Hotkey, _T("Ctrl+O"));
    FileMenuEntries[MenuIndex].NotifyCallback = EditOpenButtonClicked;

    MenuIndex++;
    YoriLibConstantString(&FileMenuEntries[MenuIndex].Caption, _T("&Save"));
    FileMenuEntries[MenuIndex].NotifyCallback = EditSaveButtonClicked;
    YoriLibConstantString(&FileMenuEntries[MenuIndex].Hotkey, _T("Ctrl+S"));

    MenuIndex++;
    YoriLibConstantString(&FileMenuEntries[MenuIndex].Caption, _T("Save &As..."));
    FileMenuEntries[MenuIndex].NotifyCallback = EditSaveAsButtonClicked;

    MenuIndex++;
    FileMenuEntries[MenuIndex].Flags = YORI_WIN_MENU_ENTRY_SEPERATOR;

    MenuIndex++;
    YoriLibConstantString(&FileMenuEntries[MenuIndex].Caption, _T("E&xit"));
    YoriLibConstantString(&FileMenuEntries[MenuIndex].Hotkey, _T("Ctrl+Q"));
    FileMenuEntries[MenuIndex].NotifyCallback = EditExitButtonClicked;

    MenuIndex = 0;
    ZeroMemory(&EditMenuEntries, sizeof(EditMenuEntries));

    EditContext->EditUndoMenuIndex = MenuIndex;
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Caption, _T("&Undo"));
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Hotkey, _T("Ctrl+Z"));
    EditMenuEntries[MenuIndex].NotifyCallback = EditUndoButtonClicked;

    MenuIndex++;
    EditContext->EditRedoMenuIndex = MenuIndex;
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Caption, _T("&Redo"));
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Hotkey, _T("Ctrl+R"));
    EditMenuEntries[MenuIndex].NotifyCallback = EditRedoButtonClicked;

    MenuIndex++;
    EditMenuEntries[MenuIndex].Flags = YORI_WIN_MENU_ENTRY_SEPERATOR;

    MenuIndex++;
    EditContext->EditCutMenuIndex = MenuIndex;
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Caption, _T("Cu&t"));
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Hotkey, _T("Ctrl+X"));
    EditMenuEntries[MenuIndex].NotifyCallback = EditCutButtonClicked;

    MenuIndex++;
    EditContext->EditCopyMenuIndex = MenuIndex;
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Caption, _T("&Copy"));
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Hotkey, _T("Ctrl+C"));
    EditMenuEntries[MenuIndex].NotifyCallback = EditCopyButtonClicked;

    MenuIndex++;
    EditContext->EditPasteMenuIndex = MenuIndex;
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Caption, _T("&Paste"));
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Hotkey, _T("Ctrl+V"));
    EditMenuEntries[MenuIndex].NotifyCallback = EditPasteButtonClicked;

    MenuIndex++;
    EditContext->EditClearMenuIndex = MenuIndex;
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Caption, _T("Cl&ear"));
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Hotkey, _T("Del"));
    EditMenuEntries[MenuIndex].NotifyCallback = EditClearButtonClicked;

    ZeroMemory(&SearchMenuEntries, sizeof(SearchMenuEntries));
    MenuIndex = 0;
    YoriLibConstantString(&SearchMenuEntries[MenuIndex].Caption, _T("&Find..."));
    YoriLibConstantString(&SearchMenuEntries[MenuIndex].Hotkey, _T("Ctrl+F"));
    SearchMenuEntries[MenuIndex].NotifyCallback = EditFindButtonClicked;

    MenuIndex++;
    YoriLibConstantString(&SearchMenuEntries[MenuIndex].Caption, _T("&Repeat Last Find"));
    YoriLibConstantString(&SearchMenuEntries[MenuIndex].Hotkey, _T("F3"));
    SearchMenuEntries[MenuIndex].NotifyCallback = EditFindNextButtonClicked;

    MenuIndex++;
    YoriLibConstantString(&SearchMenuEntries[MenuIndex].Caption, _T("Find &Previous"));
    YoriLibConstantString(&SearchMenuEntries[MenuIndex].Hotkey, _T("Shift+F3"));
    SearchMenuEntries[MenuIndex].NotifyCallback = EditFindPreviousButtonClicked;

    MenuIndex++;
    YoriLibConstantString(&SearchMenuEntries[MenuIndex].Caption, _T("&Change..."));
    SearchMenuEntries[MenuIndex].NotifyCallback = EditChangeButtonClicked;

    MenuIndex++;
    SearchMenuEntries[MenuIndex].Flags = YORI_WIN_MENU_ENTRY_SEPERATOR;

    MenuIndex++;
    YoriLibConstantString(&SearchMenuEntries[MenuIndex].Caption, _T("&Go to line..."));
    YoriLibConstantString(&SearchMenuEntries[MenuIndex].Hotkey, _T("Ctrl+G"));
    SearchMenuEntries[MenuIndex].NotifyCallback = EditGoToLineButtonClicked;

    ZeroMemory(&OptionsMenuEntries, sizeof(OptionsMenuEntries));
    MenuIndex = 0;
    YoriLibConstantString(&OptionsMenuEntries[MenuIndex].Caption, _T("&Display..."));
    OptionsMenuEntries[MenuIndex].NotifyCallback = EditDisplayOptionsButtonClicked;

    MenuIndex++;
    if (EditContext->TraditionalNavigation) {
        OptionsMenuEntries[MenuIndex].Flags = YORI_WIN_MENU_ENTRY_CHECKED;
    }
    YoriLibConstantString(&OptionsMenuEntries[MenuIndex].Caption, _T("&Traditional navigation"));
    OptionsMenuEntries[MenuIndex].NotifyCallback = EditTraditionalNavigationOptionsButtonClicked;
    EditContext->OptionsTraditionalMenuIndex = MenuIndex;

    ZeroMemory(&HelpMenuEntries, sizeof(HelpMenuEntries));
    MenuIndex = 0;
    YoriLibConstantString(&HelpMenuEntries[MenuIndex].Caption, _T("&About..."));
    HelpMenuEntries[MenuIndex].NotifyCallback = EditAboutButtonClicked;

    MenuBarItems.ItemCount = 5;
    MenuBarItems.Items = MenuEntries;

    MenuIndex = 0;
    ZeroMemory(MenuEntries, sizeof(MenuEntries));
    YoriLibConstantString(&MenuEntries[MenuIndex].Caption, _T("&File"));
    MenuEntries[MenuIndex].ChildMenu.ItemCount = sizeof(FileMenuEntries)/sizeof(FileMenuEntries[0]);
    MenuEntries[MenuIndex].ChildMenu.Items = FileMenuEntries;

    MenuIndex++;
    EditContext->EditMenuIndex = MenuIndex;
    YoriLibConstantString(&MenuEntries[MenuIndex].Caption, _T("&Edit"));
    MenuEntries[MenuIndex].NotifyCallback = EditEditButtonClicked;
    MenuEntries[MenuIndex].ChildMenu.ItemCount = sizeof(EditMenuEntries)/sizeof(EditMenuEntries[0]);
    MenuEntries[MenuIndex].ChildMenu.Items = EditMenuEntries;

    MenuIndex++;
    YoriLibConstantString(&MenuEntries[MenuIndex].Caption, _T("&Search"));
    MenuEntries[MenuIndex].ChildMenu.ItemCount = sizeof(SearchMenuEntries)/sizeof(SearchMenuEntries[0]);
    MenuEntries[MenuIndex].ChildMenu.Items = SearchMenuEntries;

    MenuIndex++;
    EditContext->OptionsMenuIndex = MenuIndex;
    YoriLibConstantString(&MenuEntries[MenuIndex].Caption, _T("&Options"));
    MenuEntries[MenuIndex].ChildMenu.ItemCount = sizeof(OptionsMenuEntries)/sizeof(OptionsMenuEntries[0]);
    MenuEntries[MenuIndex].ChildMenu.Items = OptionsMenuEntries;

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
 The minimum width in characters where edit can hope to function.
 */
#define EDIT_MINIMUM_WIDTH (60)

/**
 The minimum height in characters where edit can hope to function.
 */
#define EDIT_MINIMUM_HEIGHT (20)

/**
 A callback that is invoked when the window manager is being resized.  This
 typically means the user resized the window.  Since the purpose of edit is
 to fully occupy the window space, this implies the 
 */
VOID
EditResizeWindowManager(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle,
    __in PSMALL_RECT OldPosition,
    __in PSMALL_RECT NewPosition
    )
{
    PEDIT_CONTEXT EditContext;
    PYORI_WIN_CTRL_HANDLE WindowCtrl;
    SMALL_RECT Rect;
    COORD NewSize;

    UNREFERENCED_PARAMETER(OldPosition);

    WindowCtrl = YoriWinGetCtrlFromWindow(WindowHandle);
    EditContext = YoriWinGetControlContext(WindowCtrl);

    NewSize.X = (SHORT)(NewPosition->Right - NewPosition->Left + 1);
    NewSize.Y = (SHORT)(NewPosition->Bottom - NewPosition->Top + 1);

    if (NewSize.X < EDIT_MINIMUM_WIDTH || NewSize.Y < EDIT_MINIMUM_HEIGHT) {
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

    YoriWinMenuBarReposition(EditContext->MenuBar, &Rect);

    YoriWinGetClientSize(WindowHandle, &NewSize);

    Rect.Left = 0;
    Rect.Top = 0;
    Rect.Right = (SHORT)(NewSize.X - 1);
    Rect.Bottom = (SHORT)(NewSize.Y - 2);

    YoriWinMultilineEditReposition(EditContext->MultilineEdit, &Rect);

    Rect.Left = 0;
    Rect.Top = (SHORT)(NewSize.Y - 1);
    Rect.Right = (SHORT)(NewSize.X - 1);
    Rect.Bottom = (SHORT)(NewSize.Y - 1);

    YoriWinLabelReposition(EditContext->StatusBar, &Rect);
}


/**
 Display a popup window containing a list of items.

 @return TRUE to indicate that the user successfully selected an option, FALSE
         to indicate the menu could not be displayed or the user cancelled the
         operation.
 */
__success(return)
BOOL
EditCreateMainWindow(
    __in PEDIT_CONTEXT EditContext
    )
{
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgr;
    PYORI_WIN_CTRL_HANDLE MultilineEdit;
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

    YoriWinMgrSetAsciiDrawing(WinMgr, EditContext->UseAsciiDrawing);

    if (!YoriWinGetWinMgrDimensions(WinMgr, &WindowSize)) {
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    if (WindowSize.X < 60 || WindowSize.Y < 20) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("edit: window size too small\n"));
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    if (!YoriWinCreateWindow(WinMgr, WindowSize.X, WindowSize.Y, WindowSize.X, WindowSize.Y, 0, NULL, &Parent)) {
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    MenuBar = EditPopulateMenuBar(EditContext, Parent);
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

    MultilineEdit = YoriWinMultilineEditCreate(Parent, NULL, &Rect, YORI_WIN_MULTILINE_EDIT_STYLE_VSCROLLBAR);
    if (MultilineEdit == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriWinMultilineEditSetTraditionalNavigation(MultilineEdit, EditContext->TraditionalNavigation);

    Rect.Top = (SHORT)(Rect.Bottom + 1);
    Rect.Bottom = Rect.Top;

    YoriLibInitEmptyString(&Caption);

    StatusBar = YoriWinLabelCreate(Parent, &Rect, &Caption, YORI_WIN_LABEL_STYLE_RIGHT_ALIGN);
    if (StatusBar == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriWinLabelSetTextAttributes(StatusBar, BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);

    EditContext->WinMgr = WinMgr;
    EditContext->MultilineEdit = MultilineEdit;
    EditContext->MenuBar = MenuBar;
    EditContext->StatusBar = StatusBar;

    YoriWinMultilineEditSetColor(MultilineEdit, BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    YoriWinMultilineEditSetCursorMoveNotifyCallback(MultilineEdit, EditNotifyCursorMove);

    YoriWinSetWindowManagerResizeNotifyCallback(Parent, EditResizeWindowManager);

    if (EditContext->OpenFileName.StartOfString != NULL) {
        EditLoadFile(EditContext, &EditContext->OpenFileName);
        EditUpdateOpenedFileCaption(EditContext);
    }

    YoriWinSetControlContext(Parent, EditContext);
    EditNotifyCursorMove(MultilineEdit, 0, 0);

    Result = FALSE;
    if (!YoriWinProcessInputForWindow(Parent, &Result)) {
        Result = FALSE;
    }

    YoriWinDestroyWindow(Parent);
    YoriWinCloseWindowManager(WinMgr);
    return (BOOL)Result;
}

/**
 Parse a user specified argument into an encoding identifier.

 @param String The string to parse.

 @return The encoding identifier.  -1 is used to indicate failure.
 */
DWORD
EditEncodingFromString(
    __in PYORI_STRING String
    )
{
    if (YoriLibCompareStringWithLiteralInsensitive(String, _T("utf8")) == 0 &&
        YoriLibIsUtf8Supported()) {
        return CP_UTF8;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("ascii")) == 0) {
        return CP_OEMCP;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("ansi")) == 0) {
        return CP_ACP;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("utf16")) == 0) {
        return CP_UTF16;
    }
    return (DWORD)-1;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the edit builtin command.
 */
#define ENTRYPOINT YoriCmd_YEDIT
#else
/**
 The main entrypoint for the edit standalone application.
 */
#define ENTRYPOINT ymain
#endif


/**
 Display yori shell editor.

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

    ZeroMemory(&GlobalEditContext, sizeof(GlobalEditContext));
    if (YoriLibIsUtf8Supported()) {
        GlobalEditContext.Encoding = CP_UTF8_OR_16;
    } else {
        GlobalEditContext.Encoding = CP_ACP;
    }

    GlobalEditContext.TraditionalNavigation = TRUE;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                EditHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(strCopyrightYear);
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("a")) == 0) {
                GlobalEditContext.UseAsciiDrawing = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("e")) == 0) {
                if (ArgC > i + 1) {
                    DWORD NewEncoding;
                    NewEncoding = EditEncodingFromString(&ArgV[i + 1]);
                    if (NewEncoding != (DWORD)-1) {
                        GlobalEditContext.Encoding = NewEncoding;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("m")) == 0) {
                GlobalEditContext.TraditionalNavigation = FALSE;
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
        if (!YoriLibUserStringToSingleFilePath(&ArgV[StartArg], TRUE, &GlobalEditContext.OpenFileName)) {
            return EXIT_FAILURE;
        }
    }

    Result = EXIT_SUCCESS;

    if (!EditCreateMainWindow(&GlobalEditContext)) {
        Result = EXIT_FAILURE;
    }

#if !YORI_BUILTIN
    YoriLibLineReadCleanupCache();
#endif

    EditFreeEditContext(&GlobalEditContext);
    return Result;
}

// vim:sw=4:ts=4:et:
