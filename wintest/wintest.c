/**
 * @file co/co.c
 *
 * Yori shell window manager test
 *
 * Copyright (c) 2019-2021 Malcolm J. Smith
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
 The width of each button, in characters.
 */
#define WINTEST_BUTTON_WIDTH (16)

/**
 A callback invoked when the exit button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
WinTestExitButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    YoriWinCloseWindow(Parent, TRUE);
}

PYORI_WIN_WINDOW_HANDLE
WinTestCreateWindow(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgr,
    __in PCYORI_STRING Title,
    __in COORD Offset
    )
{
    PYORI_WIN_WINDOW_HANDLE Window;
    COORD WindowSize;
    SMALL_RECT ButtonArea;
    SMALL_RECT LabelArea;
    YORI_STRING Caption;
    WORD ButtonWidth;
    YORI_ALLOC_SIZE_T Index;
    YORI_ALLOC_SIZE_T StringOffset;
    YORI_ALLOC_SIZE_T WordLength;
    YORI_ALLOC_SIZE_T WordOffset;
    PYORI_WIN_CTRL_HANDLE Ctrl;

    if (!YoriWinGetWinMgrDimensions(WinMgr, &WindowSize)) {
        return NULL;
    }

    ButtonArea.Left = Offset.X;
    ButtonArea.Right = ButtonArea.Left + WindowSize.X / 8 * 5;
    ButtonArea.Top = Offset.Y;
    ButtonArea.Bottom = ButtonArea.Top + WindowSize.Y / 8 * 5;

    if (!YoriWinCreateWindowEx(WinMgr, &ButtonArea, YORI_WIN_WINDOW_STYLE_BORDER_SINGLE | YORI_WIN_WINDOW_STYLE_SHADOW_SOLID, Title, &Window)) {
        return NULL;
    }

    YoriWinGetClientSize(Window, &WindowSize);

    ButtonWidth = (WORD)(WINTEST_BUTTON_WIDTH);

    ButtonArea.Top = (SHORT)(1);
    ButtonArea.Bottom = (SHORT)(3);

    YoriLibConstantString(&Caption, _T("E&xit"));

    ButtonArea.Left = (SHORT)(WindowSize.X - 2 - WINTEST_BUTTON_WIDTH - 1);
    ButtonArea.Right = (WORD)(ButtonArea.Left + 1 + WINTEST_BUTTON_WIDTH);

    Ctrl = YoriWinButtonCreate(Window, &ButtonArea, &Caption, YORI_WIN_BUTTON_STYLE_CANCEL, WinTestExitButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Window);
        return NULL;
    }

    if (!YoriLibAllocateString(&Caption, 128)) {
        YoriWinDestroyWindow(Window);
        return NULL;
    }

    LabelArea.Left = 1;
    LabelArea.Top = 1;
    LabelArea.Bottom = 8;
    LabelArea.Right = (SHORT)(ButtonArea.Left - 1);

    //
    //  Create some double wide chars, normal chars, and an accelerator
    //  in the middle
    //
    StringOffset = 0;
    for (Index = 1; Index < 12; Index++) {
        WordLength = Index;
        if (WordLength > 6) {
            WordLength = WordLength - 6;
        }
        for (WordOffset = 0; WordOffset < WordLength && StringOffset < Caption.LengthAllocated; WordOffset++) {
            if (WordLength == 5 && WordOffset == 2 && StringOffset + 1 < Caption.LengthAllocated) {
                Caption.StartOfString[StringOffset] = '&';
                StringOffset++;
            }
            Caption.StartOfString[StringOffset] = 0x3405;
            StringOffset++;
        }

        if (StringOffset < Caption.LengthAllocated) {
            Caption.StartOfString[StringOffset] = ' ';
            StringOffset++;
        }

        for (WordOffset = 0; WordOffset < WordLength && StringOffset < Caption.LengthAllocated; WordOffset++) {
            Caption.StartOfString[StringOffset] = 'x';
            StringOffset++;
        }

        if (StringOffset < Caption.LengthAllocated) {
            Caption.StartOfString[StringOffset] = ' ';
            StringOffset++;
        }
    }
    Caption.LengthInChars = StringOffset;

    Ctrl = YoriWinLabelCreate(Window, &LabelArea, &Caption, 0);
    YoriLibFreeStringContents(&Caption);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Window);
        return NULL;
    }

    return Window;
}


__success(return)
BOOL
WinTest(VOID)
{
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgr;
    PYORI_WIN_WINDOW_HANDLE Window1;
    PYORI_WIN_WINDOW_HANDLE Window2;
    COORD Offset;
    DWORD_PTR Result;
    YORI_STRING Title;

    if (!YoriWinOpenWindowManager(FALSE, YoriWinColorTableDefault, &WinMgr)) {
        return FALSE;
    }

    Offset.X = 5;
    Offset.Y = 3;

    YoriLibConstantString(&Title, _T("Window 1"));

    Window1 = WinTestCreateWindow(WinMgr, &Title, Offset);
    if (Window1 == NULL) {
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    Offset.X = 12;
    Offset.Y = 8;

    YoriLibConstantString(&Title, _T("Window 2"));

    Window2 = WinTestCreateWindow(WinMgr, &Title, Offset);
    if (Window2 == NULL) {
        YoriWinDestroyWindow(Window1);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    Result = FALSE;
    if (!YoriWinMgrProcessAllEvents(WinMgr)) {
        Result = FALSE;
    }

    YoriWinDestroyWindow(Window2);
    YoriWinDestroyWindow(Window1);
    YoriWinCloseWindowManager(WinMgr);
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
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    UNREFERENCED_PARAMETER(ArgC);
    UNREFERENCED_PARAMETER(ArgV);
    if (!WinTest()) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
