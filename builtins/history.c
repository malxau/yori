/**
 * @file builtins/history.c
 *
 * Yori shell history output
 *
 * Copyright (c) 2018-2019 Malcolm J. Smith
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
BOOL
HistoryCreateSynchronousMenu(
    __in PYORI_STRING MenuOptions,
    __in DWORD NumberOptions,
    __out PDWORD ActiveOption
    )
{
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgr;
    PYORI_WIN_CTRL_HANDLE List;
    PYORI_WIN_WINDOW_HANDLE Parent;
    SMALL_RECT ListRect;
    COORD WindowSize;
    YORI_STRING Title;
    SMALL_RECT ButtonArea;
    YORI_STRING Caption;
    WORD ButtonWidth;
    PYORI_WIN_CTRL_HANDLE Ctrl;
    DWORD_PTR Result;

    if (NumberOptions == 0) {
        return FALSE;
    }

    if (!YoriWinOpenWindowManager(FALSE, &WinMgr)) {
        return FALSE;
    }

    YoriLibConstantString(&Title, _T("History"));

    if (!YoriWinCreateWindow(WinMgr, 30, 12, 60, 18, YORI_WIN_WINDOW_STYLE_BORDER_SINGLE | YORI_WIN_WINDOW_STYLE_SHADOW_SOLID, &Title, &Parent)) {
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriWinGetClientSize(Parent, &WindowSize);

    ListRect.Left = 1;
    ListRect.Top = 1;
    ListRect.Right = (SHORT)(WindowSize.X - 2);
    ListRect.Bottom = (SHORT)(WindowSize.Y - 3 - 1);

    List = YoriWinListCreate(Parent, &ListRect, YORI_WIN_LIST_STYLE_VSCROLLBAR);
    if (List == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    if (!YoriWinListAddItems(List, MenuOptions, NumberOptions)) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriWinListSetActiveOption(List, NumberOptions - 1);

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

    Ctrl = YoriWinButtonCreate(Parent, &ButtonArea, &Caption, YORI_WIN_BUTTON_STYLE_DEFAULT, HistoryOkButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriLibConstantString(&Caption, _T("&Cancel"));

    ButtonArea.Left = (SHORT)(WindowSize.X - 1 - (ButtonWidth + 2));
    ButtonArea.Right = (WORD)(ButtonArea.Left + ButtonWidth + 1);

    Ctrl = YoriWinButtonCreate(Parent, &ButtonArea, &Caption, YORI_WIN_BUTTON_STYLE_CANCEL, HistoryCancelButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

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

    YoriLibLineReadClose(LineContext);
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
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 0;
    DWORD LineCount = 0;
    YORI_STRING Arg;
    YORI_STRING HistoryStrings;
    PYORI_STRING SourceFile = NULL;
    LPTSTR ThisVar;
    DWORD VarLen;
    HISTORY_OP Op;

    Op = HistoryDisplayHistory;
    YoriLibLoadNtDllFunctions();
    YoriLibLoadKernel32Functions();

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                HistoryHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                Op = HistoryClearHistory;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("l")) == 0) {
                if (ArgC > i + 1) {
                    Op = HistoryLoadHistory;
                    SourceFile = &ArgV[i + 1];
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("n")) == 0) {
                if (ArgC > i + 1) {
                    DWORD CharsConsumed;
                    LONGLONG llTemp;
                    if (YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed) &&
                        CharsConsumed > 0) {

                        LineCount = (DWORD)llTemp;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("u")) == 0) {

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
        if (!HistoryLoadHistoryFromFile(&FileName)) {
            YoriLibFreeStringContents(&FileName);
            return EXIT_FAILURE;
        }
        YoriLibFreeStringContents(&FileName);
    } else if (Op == HistoryDisplayHistory) {
        if (YoriCallGetHistoryStrings(LineCount, &HistoryStrings)) {
            ThisVar = HistoryStrings.StartOfString;
            while (*ThisVar != '\0') {
                VarLen = _tcslen(ThisVar);
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%s\n"), ThisVar);
                ThisVar += VarLen;
                ThisVar++;
            }
            YoriCallFreeYoriString(&HistoryStrings);
        }
    } else if (Op == HistoryDisplayUi) {
        if (YoriCallGetHistoryStrings(LineCount, &HistoryStrings)) {
            PYORI_STRING StringArray;
            DWORD StringCount;
            DWORD Index;
            StringCount = 0;
            ThisVar = HistoryStrings.StartOfString;
            while (*ThisVar != '\0') {
                VarLen = _tcslen(ThisVar);
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
                VarLen = _tcslen(ThisVar);
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
