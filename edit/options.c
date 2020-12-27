/**
 * @file edit/options.c
 *
 * Yori shell editor options dialog
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

/**
 A callback invoked when the ok button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
EditOptsOkButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    YoriWinCloseWindow(Parent, TRUE);
}

/**
 A callback invoked when the cancel button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
EditOptsCancelButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    YoriWinCloseWindow(Parent, FALSE);
}

/**
 Display a dialog box to allow the user to configure the display for the
 edit application.

 @param WinMgrHandle Pointer to the window manager.

 @param InitialTabWidth Specifies the current tab width to populate into
        the dialog.

 @param NewTabWidth On successful completion, populated with the new tab
        width to display.

 @return TRUE to success, FALSE to indicate failure including the user
         pressing the cancel button.
 */
__success(return)
BOOLEAN
EditOpts(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in DWORD InitialTabWidth,
    __out PDWORD NewTabWidth
    )
{
    PYORI_WIN_WINDOW_HANDLE Parent;
    COORD WindowSize;
    SMALL_RECT Area;
    YORI_STRING Caption;
    TCHAR CaptionBuffer[16];
    WORD ButtonWidth;
    PYORI_WIN_CTRL_HANDLE Ctrl;
    PYORI_WIN_CTRL_HANDLE Edit;
    DWORD_PTR Result;

    YoriLibConstantString(&Caption, _T("Options"));

    if (!YoriWinCreateWindow(WinMgrHandle, 40, 11, 40, 11, YORI_WIN_WINDOW_STYLE_BORDER_SINGLE | YORI_WIN_WINDOW_STYLE_SHADOW_SOLID, &Caption, &Parent)) {
        return FALSE;
    }

    YoriWinGetClientSize(Parent, &WindowSize);

    YoriLibConstantString(&Caption, _T("&Tab width:"));

    Area.Left = 1;
    Area.Top = 2;
    Area.Right = (WORD)(Area.Left + Caption.LengthInChars);
    Area.Bottom = Area.Top;

    Ctrl = YoriWinLabelCreate(Parent, &Area, &Caption, 0);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Area.Top = 1;
    Area.Bottom = 3;
    Area.Left = (WORD)(Area.Right + 2);
    Area.Right = (WORD)(WindowSize.X - 2);

    Caption.StartOfString = CaptionBuffer;
    Caption.LengthAllocated = sizeof(CaptionBuffer)/sizeof(CaptionBuffer[0]);

    YoriLibYPrintf(&Caption, _T("%i"), InitialTabWidth);
    Edit = YoriWinEditCreate(Parent, &Area, &Caption, 0);
    if (Edit == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    YoriWinEditSetSelectionRange(Edit, 0, Caption.LengthInChars);

    ButtonWidth = (WORD)(8);

    Area.Top = (SHORT)(5);
    Area.Bottom = (SHORT)(7);

    YoriLibConstantString(&Caption, _T("&Ok"));

    Area.Left = (SHORT)(1);
    Area.Right = (WORD)(Area.Left + 1 + ButtonWidth);

    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, YORI_WIN_BUTTON_STYLE_DEFAULT, EditOptsOkButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Area.Left = (WORD)(Area.Left + ButtonWidth + 3);
    Area.Right = (WORD)(Area.Right + ButtonWidth + 3);

    YoriLibConstantString(&Caption, _T("&Cancel"));
    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, YORI_WIN_BUTTON_STYLE_CANCEL, EditOptsCancelButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Result = FALSE;
    if (!YoriWinProcessInputForWindow(Parent, &Result)) {
        Result = FALSE;
    }

    if (Result) {
        LONGLONG llTemp;
        DWORD CharsConsumed;

        Caption.StartOfString = CaptionBuffer;
        Caption.LengthAllocated = sizeof(CaptionBuffer)/sizeof(CaptionBuffer[0]);

        if (!YoriWinEditGetText(Edit, &Caption)) {
            Result = FALSE;
            Caption.LengthInChars = 0;
        }

        if (YoriLibStringToNumber(&Caption, FALSE, &llTemp, &CharsConsumed) &&
            CharsConsumed > 0 &&
            llTemp <= 64 &&
            llTemp >= 0) {

            DWORD dwTemp;
            dwTemp = (DWORD)llTemp;

            *NewTabWidth = dwTemp;
        } else {

            YORI_STRING Title;
            YORI_STRING Text;
            YORI_STRING ButtonText;

            YoriLibConstantString(&Title, _T("Error"));
            YoriLibConstantString(&Text, _T("Invalid tab width."));
            YoriLibConstantString(&ButtonText, _T("&Ok"));

            YoriDlgMessageBox(WinMgrHandle,
                              &Title,
                              &Text,
                              1,
                              &ButtonText,
                              0,
                              0);

            Result = FALSE;
        }
    }

    YoriLibFreeStringContents(&Caption);
    YoriWinDestroyWindow(Parent);
    return (BOOLEAN)Result;
}

// vim:sw=4:ts=4:et:
