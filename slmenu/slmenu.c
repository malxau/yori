/**
 * @file slmenu/slmenu.c
 *
 * Yori shell single line menu
 *
 * Copyright (c) 2018-2024 Malcolm J. Smith
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
CHAR strSlmenuHelpText[] =
        "\n"
        "Displays a menu based on standard input and displays selection to output."
        "\n"
        "SLMENU [-license] [-b|-t|-l <lines>|-f] [-p <text>]\n"
        "\n"
        "   -b             Display single line at the bottom of the window\n"
        "   -f             Display a full screen list\n"
        "   -l <lines>     Display in multiple lines and specify the number of lines\n" 
        "   -p <text>      Display prompt text before items\n" 
        "   -t             Display single line at the top of the window\n";

/**
 Display usage text to the user.
 */
BOOL
SlmenuHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Slmenu %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strSlmenuHelpText);
    return TRUE;
}

/**
 Context information passed between reading the input stream (including all
 lines to populate) and menu display.
 */
typedef struct _SLMENU_CONTEXT {

    /**
     An array of strings to populate into to the menu.
     */
    PYORI_STRING StringArray;

    /**
     The number of elements allocated in StringArray.
     */
    YORI_ALLOC_SIZE_T StringsAllocated;

    /**
     The number of strings populated in the StringArray.
     */
    YORI_ALLOC_SIZE_T StringCount;

    /**
     The string that was most recently searched for.
     */
    YORI_STRING SearchString;

} SLMENU_CONTEXT, *PSLMENU_CONTEXT;

/**
 A list of well known control IDs.
 */
typedef enum _SLMENU_CONTROLS {
    SlmenuControlList = 1
} SLMENU_CONTROLS;

/**
 A callback invoked when the ok button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
SlmenuOkButtonClicked(
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
SlmenuCancelButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    YoriWinCloseWindow(Parent, FALSE);
}

/**
 A callback invoked when the find button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
SlmenuFindButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PYORI_WIN_CTRL_HANDLE List;
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgr;
    PSLMENU_CONTEXT MenuContext;
    YORI_STRING Title;
    BOOLEAN MatchCase;
    YORI_STRING Text;
    YORI_ALLOC_SIZE_T ActiveIndex;
    YORI_ALLOC_SIZE_T SearchIndex;

    Parent = YoriWinGetControlParent(Ctrl);
    WinMgr = YoriWinGetWindowManagerHandle(Parent);
    MenuContext = YoriWinGetControlContext(Parent);

    YoriLibConstantString(&Title, _T("Find"));
    YoriLibInitEmptyString(&Text);

    if (!YoriDlgFindText(WinMgr,
                         &Title,
                         &MenuContext->SearchString,
                         &MatchCase,
                         &Text)) {

        return;
    }

    List = YoriWinFindControlById(Parent, SlmenuControlList);
    ASSERT(List != NULL);

    //
    //  If nothing is selected, start from the top, and search to the end.
    //

    if (!YoriWinListGetActiveOption(List, &ActiveIndex)) {
        for (SearchIndex = 0; SearchIndex < MenuContext->StringCount; SearchIndex++) {
            if (YoriLibFindFirstMatchSubstrIns(&MenuContext->StringArray[SearchIndex], 1, &Text, NULL) != NULL) {
                break;
            }
        }

    } else {

        //
        //  If something is selected, search from the next item to the end.
        //  If it's still not found, start from the top up to the selected
        //  item.
        //

        for (SearchIndex = ActiveIndex + 1; SearchIndex < MenuContext->StringCount; SearchIndex++) {
            if (YoriLibFindFirstMatchSubstrIns(&MenuContext->StringArray[SearchIndex], 1, &Text, NULL) != NULL) {
                break;
            }
        }

        if (SearchIndex == MenuContext->StringCount) {
            for (SearchIndex = 0; SearchIndex < ActiveIndex; SearchIndex++) {
                if (YoriLibFindFirstMatchSubstrIns(&MenuContext->StringArray[SearchIndex], 1, &Text, NULL) != NULL) {
                    break;
                }
            }
        }
    }

    if (SearchIndex < MenuContext->StringCount) {
        YoriWinListSetActiveOption(List, SearchIndex);
    }

    YoriLibFreeStringContents(&MenuContext->SearchString);
    YoriLibCloneString(&MenuContext->SearchString, &Text);
    YoriLibFreeStringContents(&Text);
}

/**
 The set of locations where a single line menu can be displayed.
 */
typedef enum _SLMENU_LOCATION {
    SlmenuCurrentLine = 0,
    SlmenuTopLine = 1,
    SlmenuBottomLine = 2,
    SlmenuCurrentLineRemainder = 3
} SLMENU_LOCATION;

/**
 Given an array of items and a control width, calculate the "best" size for
 each item.  If everything fits, this is the same as the longest item.  If
 it doesn't fit, truncate up to twice the average size.  Recalculate the
 width based on the actual control size to ensure that any extra cells are
 allocated to each item as much as possible.

 @param MenuOptions Pointer to an array of item strings.

 @param NumberOptions The number of strings.

 @param ControlWidth The width of the list control, in characters.

 @return The width in characters of each element.
 */
WORD
SlmenuCalculateColumnWidthForItems(
    __in PYORI_STRING MenuOptions,
    __in YORI_ALLOC_SIZE_T NumberOptions,
    __in YORI_ALLOC_SIZE_T ControlWidth
    )
{
    YORI_ALLOC_SIZE_T LongestItem;
    YORI_ALLOC_SIZE_T AverageNameLength;
    DWORD TotalLength;
    YORI_ALLOC_SIZE_T ColumnWidth;
    YORI_ALLOC_SIZE_T ColumnCount;

    YORI_ALLOC_SIZE_T Index;

    TotalLength = 0;
    LongestItem = 0;
    for (Index = 0; Index < NumberOptions; Index++) {
        TotalLength = TotalLength + MenuOptions[Index].LengthInChars;
        if (MenuOptions[Index].LengthInChars > LongestItem) {
            LongestItem = MenuOptions[Index].LengthInChars;
        }
    }

    AverageNameLength = (YORI_ALLOC_SIZE_T)(TotalLength / NumberOptions);

    //
    //  The item width contains two padding characters, so add those here.
    //

    ColumnWidth = LongestItem + 2;

    if (ColumnWidth * NumberOptions > ControlWidth) {
        if (LongestItem > 2 * AverageNameLength) {
            ColumnWidth = 2 * AverageNameLength;
        }
    }

    ColumnCount = ControlWidth / ColumnWidth;
    if (ColumnCount == 0) {
        ColumnCount = 1;
    }

    ColumnWidth = ControlWidth / ColumnCount;
    return (WORD)ColumnWidth;
}

/**
 Display a popup window containing a list of items.

 @param MenuContext Pointer to the application context.

 @param Location The location to display the line.

 @param Title The string to display on the title bar of the dialog.

 @param ActiveOption On successful completion, updated to contain the option
        that the user selected.

 @return TRUE to indicate that the user successfully selected an option, FALSE
         to indicate the menu could not be displayed or the user cancelled the
         operation.
 */
__success(return)
BOOL
SlmenuCreateSinglelineMenu(
    __in PSLMENU_CONTEXT MenuContext,
    __in SLMENU_LOCATION Location,
    __in_opt PYORI_STRING Title,
    __out PYORI_ALLOC_SIZE_T ActiveOption
    )
{
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgr;
    PYORI_WIN_CTRL_HANDLE List;
    PYORI_WIN_CTRL_HANDLE Label;
    PYORI_WIN_WINDOW_HANDLE Parent;
    SMALL_RECT CtrlRect;
    COORD WindowSize;
    YORI_STRING Caption;
    WORD ButtonWidth;
    PYORI_WIN_CTRL_HANDLE Ctrl;
    DWORD_PTR Result;
    COORD WinMgrSize;

    if (MenuContext->StringCount == 0) {
        return FALSE;
    }

    if (!YoriWinOpenWindowManager(FALSE, YoriWinColorTableDefault, &WinMgr)) {
        return FALSE;
    }

    if (!YoriWinGetWinMgrDimensions(WinMgr, &WinMgrSize)) {
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    CtrlRect.Top = 0;
    CtrlRect.Bottom = 0;
    CtrlRect.Left = 0;
    CtrlRect.Right = (SHORT)(WinMgrSize.X - 1);

    if (Location == SlmenuBottomLine) {
        CtrlRect.Top = (SHORT)(WinMgrSize.Y - 1);
        CtrlRect.Bottom = (SHORT)(WinMgrSize.Y - 1);
    } else if (Location == SlmenuCurrentLine ||
               Location == SlmenuCurrentLineRemainder) {
        COORD CursorLocation;
        SMALL_RECT WinMgrPos;

        if (!YoriWinGetWinMgrLocation(WinMgr, &WinMgrPos)) {
            YoriWinCloseWindowManager(WinMgr);
            return FALSE;
        }

        if (!YoriWinGetWinMgrInitialCursorLocation(WinMgr, &CursorLocation)) {
            YoriWinCloseWindowManager(WinMgr);
            return FALSE;
        }

        if (CursorLocation.Y < WinMgrPos.Top || CursorLocation.Y > WinMgrPos.Bottom) {
            YoriWinCloseWindowManager(WinMgr);
            return FALSE;
        }

        CtrlRect.Top = (SHORT)(CursorLocation.Y - WinMgrPos.Top);
        CtrlRect.Bottom = (SHORT)(CursorLocation.Y - WinMgrPos.Top);

        if (Location == SlmenuCurrentLineRemainder) {
            CtrlRect.Left = (SHORT)(CursorLocation.X - WinMgrPos.Left);
        }
    }

    if (!YoriWinCreateWindowEx(WinMgr, &CtrlRect, 0, NULL, &Parent)) {
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriWinGetClientSize(Parent, &WindowSize);

    CtrlRect.Left = 0;
    CtrlRect.Top = 0;
    CtrlRect.Bottom = 0;

    if (Title != NULL) {
        CtrlRect.Left = 0;
        CtrlRect.Top = 0;
        CtrlRect.Bottom = 0;
        CtrlRect.Right = (WORD)(Title->LengthInChars - 1);

        Label = YoriWinLabelCreate(Parent, &CtrlRect, Title, 0);
        CtrlRect.Left = (WORD)(CtrlRect.Right + 1);
    }

    CtrlRect.Right = (SHORT)(WindowSize.X - 1);

    List = YoriWinListCreate(Parent, &CtrlRect, YORI_WIN_LIST_STYLE_HORIZONTAL | YORI_WIN_LIST_STYLE_NO_BORDER);
    if (List == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriWinGetClientSize(List, &WindowSize);
    ButtonWidth = SlmenuCalculateColumnWidthForItems(MenuContext->StringArray, MenuContext->StringCount, WindowSize.X);
    YoriWinListSetHorizontalItemWidth(List, ButtonWidth);

    if (!YoriWinListAddItems(List, MenuContext->StringArray, MenuContext->StringCount)) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriWinListSetActiveOption(List, 0);

    ButtonWidth = (WORD)(sizeof("Cancel") - 1 + 2);

    CtrlRect.Top = (SHORT)(WindowSize.Y - 3);
    CtrlRect.Bottom = (SHORT)(CtrlRect.Top + 2);

    YoriLibConstantString(&Caption, _T("&Ok"));

    //
    //  WindowSize corresponds to dimensions, so rightmost cell is one
    //  less.  The button starts two buttons over, and each button
    //  has its client plus border chars, and there's an extra char
    //  between the buttons.
    //

    CtrlRect.Left = (SHORT)(WindowSize.X - 1 - 2 * (ButtonWidth + 2) - 1);
    CtrlRect.Right = (WORD)(CtrlRect.Left + ButtonWidth + 1);

    Ctrl = YoriWinButtonCreate(Parent, &CtrlRect, &Caption, YORI_WIN_BUTTON_STYLE_DEFAULT | YORI_WIN_BUTTON_STYLE_DISABLE_FOCUS, SlmenuOkButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriLibConstantString(&Caption, _T("&Cancel"));

    CtrlRect.Left = (SHORT)(WindowSize.X - 1 - (ButtonWidth + 2));
    CtrlRect.Right = (WORD)(CtrlRect.Left + ButtonWidth + 1);

    Ctrl = YoriWinButtonCreate(Parent, &CtrlRect, &Caption, YORI_WIN_BUTTON_STYLE_CANCEL | YORI_WIN_BUTTON_STYLE_DISABLE_FOCUS, SlmenuCancelButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriWinSetControlContext(Parent, MenuContext);

    Result = FALSE;
    if (!YoriWinProcessInputForWindow(Parent, &Result)) {
        Result = FALSE;
    }
    if (Result) {
        if (!YoriWinListGetActiveOption(List, ActiveOption)) {
            Result = FALSE;
        }
    }

    YoriWinDestroyWindow(Parent);
    YoriWinCloseWindowManager(WinMgr);
    return (BOOL)Result;
}

/**
 Display a popup window containing a list of items.

 @param MenuContext Pointer to the application context.

 @param Title The string to display on the title bar of the dialog.

 @param DisplayLineCount The number of lines of options to display.  The
        dialog adds overhead on top of this count.

 @param ActiveOption On successful completion, updated to contain the option
        that the user selected.

 @return TRUE to indicate that the user successfully selected an option, FALSE
         to indicate the menu could not be displayed or the user cancelled the
         operation.
 */
__success(return)
BOOL
SlmenuCreateMultilineMenu(
    __in PSLMENU_CONTEXT MenuContext,
    __in PYORI_STRING Title,
    __in YORI_ALLOC_SIZE_T DisplayLineCount,
    __out PYORI_ALLOC_SIZE_T ActiveOption
    )
{
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgr;
    PYORI_WIN_CTRL_HANDLE List;
    PYORI_WIN_WINDOW_HANDLE Parent;
    SMALL_RECT ListRect;
    COORD WindowSize;
    SMALL_RECT ButtonArea;
    YORI_STRING Caption;
    WORD ButtonWidth;
    PYORI_WIN_CTRL_HANDLE Ctrl;
    DWORD_PTR Result;
    WORD WindowHeight;

    if (MenuContext->StringCount == 0) {
        return FALSE;
    }

    if (!YoriWinOpenWindowManager(FALSE, YoriWinColorTableDefault, &WinMgr)) {
        return FALSE;
    }

    if (DisplayLineCount == 0) {
        YoriWinGetWinMgrDimensions(WinMgr, &WindowSize);
        WindowHeight = WindowSize.Y;

        if (WindowSize.X < 40 || WindowSize.Y < 12) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("slmenu: window size too small\n"));
            YoriWinCloseWindowManager(WinMgr);
            return FALSE;
        }

        if (!YoriWinCreateWindow(WinMgr, WindowSize.X, WindowSize.Y, WindowSize.X, WindowSize.Y, 0, NULL, &Parent)) {
            YoriWinCloseWindowManager(WinMgr);
            return FALSE;
        }
    } else {
        WindowHeight = (WORD)(DisplayLineCount + 8);

        if (WindowHeight < 12) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("slmenu: window size too small\n"));
            YoriWinCloseWindowManager(WinMgr);
            return FALSE;
        }

        if (!YoriWinCreateWindow(WinMgr, 30, WindowHeight, 60, WindowHeight, YORI_WIN_WINDOW_STYLE_BORDER_SINGLE | YORI_WIN_WINDOW_STYLE_SHADOW_SOLID, Title, &Parent)) {
            YoriWinCloseWindowManager(WinMgr);
            return FALSE;
        }
    }

    YoriWinGetClientSize(Parent, &WindowSize);

    ListRect.Left = 1;
    ListRect.Top = 1;
    ListRect.Right = (SHORT)(WindowSize.X - 2);
    ListRect.Bottom = (SHORT)(WindowSize.Y - 3 - 1);

    List = YoriWinListCreate(Parent, &ListRect, YORI_WIN_LIST_STYLE_VSCROLLBAR | YORI_WIN_LIST_STYLE_AUTO_HSCROLLBAR);
    if (List == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    if (!YoriWinListAddItems(List, MenuContext->StringArray, MenuContext->StringCount)) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriWinListSetActiveOption(List, 0);
    YoriWinSetControlId(List, SlmenuControlList);

    ButtonWidth = (WORD)(sizeof("Cancel") - 1 + 2);

    ButtonArea.Top = (SHORT)(WindowSize.Y - 3);
    ButtonArea.Bottom = (SHORT)(ButtonArea.Top + 2);

    YoriLibConstantString(&Caption, _T("&Ok"));

    //
    //  WindowSize corresponds to dimensions, so rightmost cell is one
    //  less.  The button starts two buttons over, and each button
    //  has its client plus border chars, and there's an extra char
    //  between the buttons.
    //

    ButtonArea.Left = (SHORT)(WindowSize.X - 1 - 2 * (ButtonWidth + 2) - 1);
    ButtonArea.Right = (WORD)(ButtonArea.Left + ButtonWidth + 1);

    Ctrl = YoriWinButtonCreate(Parent, &ButtonArea, &Caption, YORI_WIN_BUTTON_STYLE_DEFAULT, SlmenuOkButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriLibConstantString(&Caption, _T("&Cancel"));

    ButtonArea.Left = (SHORT)(WindowSize.X - 1 - (ButtonWidth + 2));
    ButtonArea.Right = (WORD)(ButtonArea.Left + ButtonWidth + 1);

    Ctrl = YoriWinButtonCreate(Parent, &ButtonArea, &Caption, YORI_WIN_BUTTON_STYLE_CANCEL, SlmenuCancelButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    ButtonArea.Left = (SHORT)(1);
    ButtonArea.Right = (WORD)(ButtonArea.Left + ButtonWidth + 1);

    YoriLibConstantString(&Caption, _T("&Find"));

    Ctrl = YoriWinButtonCreate(Parent, &ButtonArea, &Caption, 0, SlmenuFindButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriWinSetControlContext(Parent, MenuContext);

    Result = FALSE;
    if (!YoriWinProcessInputForWindow(Parent, &Result)) {
        Result = FALSE;
    }
    if (Result) {
        if (!YoriWinListGetActiveOption(List, ActiveOption)) {
            Result = FALSE;
        }
    }

    YoriWinDestroyWindow(Parent);
    YoriWinCloseWindowManager(WinMgr);
    return (BOOL)Result;
}

/**
 Process a single opened stream, enumerating through all lines and loading
 them into an array.

 @param hSource The opened source stream.

 @param MenuContext Pointer to context information containing the lines.
 
 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SlmenuProcessStream(
    __in HANDLE hSource,
    __in PSLMENU_CONTEXT MenuContext
    )
{
    PVOID LineContext = NULL;
    DWORD Index;

    while (TRUE) {

        if (MenuContext->StringCount + 1 > MenuContext->StringsAllocated) {
            DWORD DesiredBytes;
            DWORD RequiredBytes;
            YORI_ALLOC_SIZE_T BytesToAllocate;
            YORI_ALLOC_SIZE_T NewStringsAllocated;
            PYORI_STRING NewStrings;

            RequiredBytes = MenuContext->StringsAllocated;
            if (RequiredBytes < 0x100) {
                RequiredBytes = 0x100;
            }
            DesiredBytes = RequiredBytes * 4 * sizeof(YORI_STRING);
            RequiredBytes = (RequiredBytes + 1) * sizeof(YORI_STRING);

            BytesToAllocate = YoriLibMaximumAllocationInRange(RequiredBytes, DesiredBytes);
            NewStringsAllocated = BytesToAllocate / sizeof(YORI_STRING);
            if (NewStringsAllocated == 0) {
                return FALSE;
            }


            NewStrings = YoriLibReferencedMalloc(NewStringsAllocated * sizeof(YORI_STRING));
            if (NewStrings == NULL) {
                return FALSE;
            }

            for (Index = MenuContext->StringCount; Index < NewStringsAllocated; Index++) {
                YoriLibInitEmptyString(&NewStrings[Index]);
            }

            if (MenuContext->StringCount > 0) {
                memcpy(NewStrings, MenuContext->StringArray, MenuContext->StringCount * sizeof(YORI_STRING));
            }
            if (MenuContext->StringArray != NULL) {
                YoriLibDereference(MenuContext->StringArray);
            }
            MenuContext->StringArray = NewStrings;
            MenuContext->StringsAllocated = NewStringsAllocated;
        }

        if (!YoriLibReadLineToString(&MenuContext->StringArray[MenuContext->StringCount], &LineContext, hSource)) {
            break;
        }

        MenuContext->StringCount++;
    }

    YoriLibLineReadCloseOrCache(LineContext);
    return TRUE;
}

/**
 Deallocate strings and the string array in the menu context.

 @param MenuContext Pointer to the context to clean up.
 */
VOID
SlmenuCleanupContext(
    __in PSLMENU_CONTEXT MenuContext
    )
{
    DWORD Index;

    for (Index = 0; Index < MenuContext->StringCount; Index++) {
        YoriLibFreeStringContents(&MenuContext->StringArray[Index]);
    }

    if (MenuContext->StringArray != NULL) {
        YoriLibDereference(MenuContext->StringArray);
    }

    YoriLibFreeStringContents(&MenuContext->SearchString);
}

/**
 The set of operations this module is capable of performing.
 */
typedef enum _SLMENU_OP {
    SlmenuUnknown = 0,
    SlmenuDisplayMultilineMenu = 1,
    SlmenuDisplaySinglelineMenu = 2,
} SLMENU_OP;

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the slmenu builtin command.
 */
#define ENTRYPOINT YoriCmd_SLMENU
#else
/**
 The main entrypoint for the slmenu standalone application.
 */
#define ENTRYPOINT ymain
#endif


/**
 Display selectable menu.

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
    YORI_STRING Arg;
    SLMENU_OP Op;
    SLMENU_CONTEXT MenuContext;
    SLMENU_LOCATION Location;
    YORI_STRING Prompt;
    PYORI_STRING DisplayPrompt;
    YORI_ALLOC_SIZE_T DisplayLineCount;
    YORI_ALLOC_SIZE_T Index;
    DWORD Result;

    ZeroMemory(&MenuContext, sizeof(MenuContext));
    Op = SlmenuDisplaySinglelineMenu;
    DisplayLineCount = 0;
    Location = SlmenuCurrentLineRemainder;

    YoriLibConstantString(&Prompt, _T("Menu"));
    DisplayPrompt = NULL;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                SlmenuHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2021-2024"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("b")) == 0) {
                Op = SlmenuDisplaySinglelineMenu;
                Location = SlmenuBottomLine;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("f")) == 0) {
                Op = SlmenuDisplayMultilineMenu;
                DisplayLineCount = 0;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("l")) == 0) {

                if (i + 1 < ArgC) {
                    YORI_MAX_SIGNED_T llTemp;
                    YORI_ALLOC_SIZE_T CharsConsumed;
                    if (YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed) &&
                        CharsConsumed > 0)  {

                        DisplayLineCount = (YORI_ALLOC_SIZE_T)llTemp;
                        Op = SlmenuDisplayMultilineMenu;
                        i++;
                        ArgumentUnderstood = TRUE;
                    }
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("p")) == 0) {
                if (i + 1 < ArgC) {
                    Prompt.StartOfString = ArgV[i + 1].StartOfString;
                    Prompt.LengthInChars = ArgV[i + 1].LengthInChars;
                    i++;
                    ArgumentUnderstood = TRUE;
                    DisplayPrompt = &Prompt;
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("t")) == 0) {
                Op = SlmenuDisplaySinglelineMenu;
                Location = SlmenuTopLine;
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

    Result = EXIT_SUCCESS;
    Index = 0;

    if (Op == SlmenuDisplaySinglelineMenu) {

        if (!SlmenuProcessStream(GetStdHandle(STD_INPUT_HANDLE), &MenuContext)) {
            Result = EXIT_FAILURE;
        }

        if (Result == EXIT_SUCCESS) {
            if (!SlmenuCreateSinglelineMenu(&MenuContext, Location, DisplayPrompt, &Index)) {
                Result = EXIT_FAILURE;
            }
        }

        if (Result == EXIT_SUCCESS) {
            if (Index < MenuContext.StringCount) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &MenuContext.StringArray[Index]);
            }
        }

        SlmenuCleanupContext(&MenuContext);

    } else if (Op == SlmenuDisplayMultilineMenu) {

        if (!SlmenuProcessStream(GetStdHandle(STD_INPUT_HANDLE), &MenuContext)) {
            Result = EXIT_FAILURE;
        }

        if (Result == EXIT_SUCCESS) {
            if (!SlmenuCreateMultilineMenu(&MenuContext, &Prompt, DisplayLineCount, &Index)) {
                Result = EXIT_FAILURE;
            }
        }

        if (Result == EXIT_SUCCESS) {
            if (Index < MenuContext.StringCount) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &MenuContext.StringArray[Index]);
            }
        }

        SlmenuCleanupContext(&MenuContext);
    }

#if !YORI_BUILTIN
    YoriLibLineReadCleanupCache();
#endif

    return Result;
}

// vim:sw=4:ts=4:et:
