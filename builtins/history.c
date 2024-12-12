/**
 * @file builtins/history.c
 *
 * Yori shell history output
 *
 * Copyright (c) 2018-2023 Malcolm J. Smith
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
#include <yoricall.h>
#include <yoriwin.h>

/**
 Help text to display to the user.
 */
const
CHAR strHistoryHelpText[] =
        "\n"
        "Displays or modifies recent command history.\n"
        "\n"
        "HISTORY [-license] [-c|-l <file>|-n lines]\n"
        "\n"
        "   -c             Clear current history\n"
        "   -l             Load history from a file\n"
        "   -n             The number of lines of history to output\n"
        "   -u             Display a menu for the user to select a command\n";

/**
 The set of control identifiers allocated within this program.
 */
typedef enum _HISTORY_CONTROLS {
    HistoryCtrlList = 1,
    HistoryCtrlOk = 2,
    HistoryCtrlCancel = 3,
} HISTORY_CONTROLS;

/**
 Display usage text to the user.
 */
BOOL
HistoryHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("History %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHistoryHelpText);
    return TRUE;
}

/**
 A callback invoked when the ok button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
HistoryOkButtonClicked(
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
HistoryCancelButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    YoriWinCloseWindow(Parent, FALSE);
}

/**
 The minimum width in characters where history ui can hope to function.
 */
#define HISTORY_MINIMUM_WIDTH (40)

/**
 The minimum height in characters where history ui can hope to function.
 */
#define HISTORY_MINIMUM_HEIGHT (12)

/**
 Determine the size of the window and location of all controls for a specified
 window manager size.  This allows the same layout logic to be used for
 initial creation as well as window manager resize.

 @param WindowMgrSize The dimensions of the window manager.

 @param WindowSize On completion, updated to contain the size of the main
        window.

 @param ListRect On completion, updated to contain the rect describing the
        main list in window client coordinates.

 @param OkButtonRect On completion, updated to contain the rect describing
        the ok button in window client coordinates.

 @param CancelButtonRect On completion, updated to contain the rect describing
        the cancel button in window client coordinates.
 */
VOID
HistoryGetControlRectsFromWindowManagerSize(
    __in COORD WindowMgrSize,
    __out PCOORD WindowSize,
    __out PSMALL_RECT ListRect,
    __out PSMALL_RECT OkButtonRect,
    __out PSMALL_RECT CancelButtonRect
    )
{
    COORD ClientSize;
    WORD ButtonWidth;

    WindowSize->X = (SHORT)(WindowMgrSize.X * 4 / 5);
    if (WindowSize->X < HISTORY_MINIMUM_WIDTH) {
        WindowSize->X = HISTORY_MINIMUM_WIDTH;
    }

    WindowSize->Y = (SHORT)(WindowMgrSize.Y * 3 / 4);
    if (WindowSize->Y < HISTORY_MINIMUM_HEIGHT) {
        WindowSize->Y = HISTORY_MINIMUM_HEIGHT;
    }

    ClientSize.X = (WORD)(WindowSize->X - 2);
    ClientSize.Y = (WORD)(WindowSize->Y - 2);

    ListRect->Left = 1;
    ListRect->Top = 1;
    ListRect->Right = (SHORT)(ClientSize.X - 2);
    ListRect->Bottom = (SHORT)(ClientSize.Y - 3 - 1);

    ButtonWidth = (WORD)(sizeof("Cancel") - 1 + 2);

    OkButtonRect->Top = (SHORT)(ClientSize.Y - 3);
    OkButtonRect->Bottom = (SHORT)(OkButtonRect->Top + 2);

    //
    //  WindowSize corresponds to dimensions, so rightmost cell is one
    //  less.  The button starts two buttons over, and each button
    //  has its client plus border chars, and there's an extra char
    //  between the buttons.
    //

    OkButtonRect->Left = (SHORT)(ClientSize.X - 1 - 2 * (ButtonWidth + 2) - 1);
    OkButtonRect->Right = (WORD)(OkButtonRect->Left + ButtonWidth + 1);

    CancelButtonRect->Left = (SHORT)(ClientSize.X - 1 - (ButtonWidth + 2));
    CancelButtonRect->Right = (WORD)(CancelButtonRect->Left + ButtonWidth + 1);
    CancelButtonRect->Top = OkButtonRect->Top;
    CancelButtonRect->Bottom = OkButtonRect->Bottom;
}

/**
 A callback that is invoked when the window manager is being resized.  This
 typically means the user resized the window.

 @param WindowHandle Handle to the main window.

 @param OldPosition The old dimensions of the window manager.

 @param NewPosition The new dimensions of the window manager.
 */
VOID
HistoryResizeWindowManager(
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
    SMALL_RECT OkButtonRect;
    SMALL_RECT CancelButtonRect;
    PYORI_WIN_CTRL_HANDLE Ctrl;

    UNREFERENCED_PARAMETER(OldPosition);

    WindowCtrl = YoriWinGetCtrlFromWindow(WindowHandle);

    NewSize.X = (SHORT)(NewPosition->Right - NewPosition->Left + 1);
    NewSize.Y = (SHORT)(NewPosition->Bottom - NewPosition->Top + 1);

    if (NewSize.X < HISTORY_MINIMUM_WIDTH || NewSize.Y < HISTORY_MINIMUM_HEIGHT) {
        return;
    }

    HistoryGetControlRectsFromWindowManagerSize(NewSize,
                                                &WindowSize,
                                                &ListRect,
                                                &OkButtonRect,
                                                &CancelButtonRect);

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

    Ctrl = YoriWinFindControlById(WindowHandle, HistoryCtrlList);
    ASSERT(Ctrl != NULL);
    YoriWinListReposition(Ctrl, &ListRect);

    Ctrl = YoriWinFindControlById(WindowHandle, HistoryCtrlOk);
    ASSERT(Ctrl != NULL);
    YoriWinButtonReposition(Ctrl, &OkButtonRect);

    Ctrl = YoriWinFindControlById(WindowHandle, HistoryCtrlCancel);
    ASSERT(Ctrl != NULL);
    YoriWinButtonReposition(Ctrl, &CancelButtonRect);
}


/**
 Display a popup window containing a list of items.

 @param MenuOptions Pointer to an array of list items.

 @param NumberOptions The number of elements within the array.

 @param ActiveOption On successful completion, updated to contain the option
        that the user selected.

 @return TRUE to indicate that the user successfully selected an option, FALSE
         to indicate the menu could not be displayed or the user cancelled the
         operation.
 */
__success(return)
BOOLEAN
HistoryCreateSynchronousMenu(
    __in PYORI_STRING MenuOptions,
    __in YORI_ALLOC_SIZE_T NumberOptions,
    __out PYORI_ALLOC_SIZE_T ActiveOption
    )
{
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgr;
    PYORI_WIN_CTRL_HANDLE List;
    PYORI_WIN_WINDOW_HANDLE Parent;
    COORD WindowSize;
    YORI_STRING Title;
    SMALL_RECT ListRect;
    SMALL_RECT OkButtonRect;
    SMALL_RECT CancelButtonRect;
    YORI_STRING Caption;
    PYORI_WIN_CTRL_HANDLE Ctrl;
    DWORD_PTR Result;

    if (NumberOptions == 0) {
        return FALSE;
    }

    if (!YoriWinOpenWindowManager(FALSE, YoriWinColorTableDefault, &WinMgr)) {
        return FALSE;
    }

    if (!YoriWinGetWinMgrDimensions(WinMgr, &WindowSize)) {
        WindowSize.X = 80;
        WindowSize.X = 24;
    }

    HistoryGetControlRectsFromWindowManagerSize(WindowSize,
                                                &WindowSize,
                                                &ListRect,
                                                &OkButtonRect,
                                                &CancelButtonRect);

    YoriLibConstantString(&Title, _T("History"));

    if (!YoriWinCreateWindow(WinMgr, WindowSize.X, WindowSize.Y, WindowSize.X, WindowSize.Y, YORI_WIN_WINDOW_STYLE_BORDER_SINGLE | YORI_WIN_WINDOW_STYLE_SHADOW_SOLID, &Title, &Parent)) {
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriWinGetClientSize(Parent, &WindowSize);

    List = YoriWinListCreate(Parent, &ListRect, YORI_WIN_LIST_STYLE_VSCROLLBAR);
    if (List == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }
    YoriWinSetControlId(List, HistoryCtrlList);

    if (!YoriWinListAddItems(List, MenuOptions, NumberOptions)) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriWinListSetActiveOption(List, NumberOptions - 1);

    YoriLibConstantString(&Caption, _T("&Ok"));
    Ctrl = YoriWinButtonCreate(Parent, &OkButtonRect, &Caption, YORI_WIN_BUTTON_STYLE_DEFAULT, HistoryOkButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }
    YoriWinSetControlId(Ctrl, HistoryCtrlOk);

    YoriLibConstantString(&Caption, _T("&Cancel"));
    Ctrl = YoriWinButtonCreate(Parent, &CancelButtonRect, &Caption, YORI_WIN_BUTTON_STYLE_CANCEL, HistoryCancelButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }
    YoriWinSetControlId(Ctrl, HistoryCtrlCancel);

    YoriWinSetWindowManagerResizeNotifyCallback(Parent, HistoryResizeWindowManager);

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
    if (Result) {
        return TRUE;
    }
    return FALSE;
}


/**
 Load history from a file.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
HistoryLoadHistoryFromFile(
    __in PYORI_STRING FilePath
    )
{
    HANDLE FileHandle;
    PVOID LineContext = NULL;
    YORI_STRING LineString;

    FileHandle = CreateFile(FilePath->StartOfString,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

    if (FileHandle == NULL || FileHandle == INVALID_HANDLE_VALUE) {
        DWORD LastError = GetLastError();
        if (LastError != ERROR_FILE_NOT_FOUND) {
            LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("history: open of %y failed: %s"), &FilePath, ErrText);
            YoriLibFreeWinErrorText(ErrText);
        }
        return FALSE;
    }

    YoriCallClearHistoryStrings();
    YoriLibInitEmptyString(&LineString);

    while (TRUE) {

        if (!YoriLibReadLineToString(&LineString, &LineContext, FileHandle)) {
            break;
        }

        //
        //  If we fail to add to history, stop.
        //

        if (!YoriCallAddHistoryString(&LineString)) {
            break;
        }
    }

    YoriLibLineReadCloseOrCache(LineContext);
    YoriLibFreeStringContents(&LineString);
    CloseHandle(FileHandle);
    return TRUE;
}

/**
 The set of operations this module is capable of performing.
 */
typedef enum _HISTORY_OP {
    HistoryDisplayHistory = 1,
    HistoryLoadHistory = 2,
    HistoryClearHistory = 3,
    HistoryDisplayUi = 4
} HISTORY_OP;

/**
 Display yori shell command history.

 @param ArgC The number of arguments.

 @param ArgV The argument array.

 @return ExitCode, zero for success, nonzero for failure.
 */
DWORD
YORI_BUILTIN_FN
YoriCmd_HISTORY(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 0;
    YORI_ALLOC_SIZE_T LineCount = 0;
    YORI_STRING Arg;
    YORI_STRING HistoryStrings;
    PYORI_STRING SourceFile = NULL;
    LPTSTR ThisVar;
    YORI_ALLOC_SIZE_T VarLen;
    HISTORY_OP Op;

    Op = HistoryDisplayHistory;
    YoriLibLoadNtDllFunctions();
    YoriLibLoadKernel32Functions();

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                HistoryHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("c")) == 0) {
                Op = HistoryClearHistory;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("l")) == 0) {
                if (ArgC > i + 1) {
                    Op = HistoryLoadHistory;
                    SourceFile = &ArgV[i + 1];
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("n")) == 0) {
                if (ArgC > i + 1) {
                    YORI_ALLOC_SIZE_T CharsConsumed;
                    YORI_MAX_SIGNED_T llTemp;
                    if (YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed) &&
                        CharsConsumed > 0) {

                        LineCount = (YORI_ALLOC_SIZE_T)llTemp;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("u")) == 0) {

                Op = HistoryDisplayUi;
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

    if (Op == HistoryClearHistory) {
        if (!YoriCallClearHistoryStrings()) {
            return EXIT_FAILURE;
        }
    } else if (Op == HistoryLoadHistory) {
        YORI_STRING FileName;
        if (!YoriLibUserStringToSingleFilePath(SourceFile, TRUE, &FileName)) {
            DWORD LastError = GetLastError();
            LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("history: getfullpathname of %y failed: %s"), &ArgV[StartArg], ErrText);
            YoriLibFreeWinErrorText(ErrText);
            return EXIT_FAILURE;
        }
        if (!YoriCallSetUnloadRoutine(YoriLibLineReadCleanupCache)) {
            YoriLibFreeStringContents(&FileName);
            return EXIT_FAILURE;
        }
        if (!HistoryLoadHistoryFromFile(&FileName)) {
            YoriLibFreeStringContents(&FileName);
            return EXIT_FAILURE;
        }
        YoriLibFreeStringContents(&FileName);
    } else if (Op == HistoryDisplayHistory) {
        if (YoriCallGetHistoryStrings(LineCount, &HistoryStrings)) {
            ThisVar = HistoryStrings.StartOfString;
            while (*ThisVar != '\0') {
                VarLen = (YORI_ALLOC_SIZE_T)_tcslen(ThisVar);
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%s\n"), ThisVar);
                ThisVar += VarLen;
                ThisVar++;
            }
            YoriCallFreeYoriString(&HistoryStrings);
        }
    } else if (Op == HistoryDisplayUi) {
        if (YoriCallGetHistoryStrings(LineCount, &HistoryStrings)) {
            PYORI_STRING StringArray;
            YORI_ALLOC_SIZE_T StringCount;
            YORI_ALLOC_SIZE_T Index;
            StringCount = 0;
            ThisVar = HistoryStrings.StartOfString;
            while (*ThisVar != '\0') {
                VarLen = (YORI_ALLOC_SIZE_T)_tcslen(ThisVar);
                StringCount++;
                ThisVar += VarLen;
                ThisVar++;
            }

            StringArray = YoriLibMalloc(sizeof(YORI_STRING)*StringCount);
            if (StringArray == NULL) {
                YoriCallFreeYoriString(&HistoryStrings);
                return EXIT_FAILURE;
            }

            Index = 0;
            ThisVar = HistoryStrings.StartOfString;
            while (*ThisVar != '\0') {
                VarLen = (YORI_ALLOC_SIZE_T)_tcslen(ThisVar);
                YoriLibInitEmptyString(&StringArray[Index]);
                StringArray[Index].StartOfString = ThisVar;
                StringArray[Index].LengthInChars = VarLen;
                StringArray[Index].LengthAllocated = VarLen + 1;
                Index++;
                ThisVar += VarLen;
                ThisVar++;
            }

            if (!HistoryCreateSynchronousMenu(StringArray, StringCount, &Index)) {
                YoriLibFree(StringArray);
                YoriCallFreeYoriString(&HistoryStrings);
                return EXIT_FAILURE;
            }

            //
            //  If the shell supports it, populate the next command with the
            //  string for the user to edit.  If not, just execute it
            //  verbatim.
            //

            if (!YoriCallSetNextCommand(&StringArray[Index])) {
                YoriCallExecuteExpression(&StringArray[Index]);
            }
            YoriLibFree(StringArray);
            YoriCallFreeYoriString(&HistoryStrings);
        }
    }
    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
