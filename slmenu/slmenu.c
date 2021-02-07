/**
 * @file slmenu/slmenu.c
 *
 * Yori shell single line menu
 *
 * Copyright (c) 2018-2021 Malcolm J. Smith
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
CHAR strSlmenuHelpText[] =
        "\n"
        "Displays a menu based on standard input and displays selection to output."
        "\n"
        "SLMENU [-license]\n"
        "\n";

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
    DWORD StringsAllocated;

    /**
     The number of strings populated in the StringArray.
     */
    DWORD StringCount;
} SLMENU_CONTEXT, *PSLMENU_CONTEXT;

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
 Display a popup window containing a list of items.

 @param MenuOptions Pointer to an array of list items.

 @param NumberOptions The number of elements within the array.

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
    __in PYORI_STRING MenuOptions,
    __in DWORD NumberOptions,
    __in PYORI_STRING Title,
    __in DWORD DisplayLineCount,
    __out PDWORD ActiveOption
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

    if (NumberOptions == 0) {
        return FALSE;
    }

    if (!YoriWinOpenWindowManager(FALSE, &WinMgr)) {
        return FALSE;
    }

    WindowHeight = (WORD)(DisplayLineCount + 9);

    if (!YoriWinCreateWindow(WinMgr, 30, WindowHeight, 60, WindowHeight, YORI_WIN_WINDOW_STYLE_BORDER_SINGLE | YORI_WIN_WINDOW_STYLE_SHADOW_SOLID, Title, &Parent)) {
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
            DWORD NewStringsAllocated;
            PYORI_STRING NewStrings;

            NewStringsAllocated = MenuContext->StringsAllocated * 4;
            if (NewStringsAllocated < 0x100) {
                NewStringsAllocated = 0x100;
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

    YoriLibLineReadClose(LineContext);
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
}

/**
 The set of operations this module is capable of performing.
 */
typedef enum _SLMENU_OP {
    SlmenuUnknown = 0,
    SlmenuDisplayMultilineMenu = 1,
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
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 0;
    YORI_STRING Arg;
    SLMENU_OP Op;
    SLMENU_CONTEXT MenuContext;
    YORI_STRING Prompt;
    DWORD DisplayLineCount;

    ZeroMemory(&MenuContext, sizeof(MenuContext));
    Op = SlmenuUnknown;
    DisplayLineCount = 5;

    YoriLibConstantString(&Prompt, _T("Menu"));

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                SlmenuHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2021"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("l")) == 0) {

                if (i + 1 < ArgC) {
                    LONGLONG llTemp;
                    DWORD CharsConsumed;
                    if (YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed) &&
                        CharsConsumed > 0)  {

                        DisplayLineCount = (DWORD)llTemp;
                        Op = SlmenuDisplayMultilineMenu;
                        i++;
                        ArgumentUnderstood = TRUE;
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("p")) == 0) {
                if (i + 1 < ArgC) {
                    Prompt.StartOfString = ArgV[i + 1].StartOfString;
                    Prompt.LengthInChars = ArgV[i + 1].LengthInChars;
                    i++;
                    ArgumentUnderstood = TRUE;
                }
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

    if (Op == SlmenuDisplayMultilineMenu) {

        DWORD Index;

        if (!SlmenuProcessStream(GetStdHandle(STD_INPUT_HANDLE), &MenuContext)) {
            SlmenuCleanupContext(&MenuContext);
            return EXIT_FAILURE;
        }

        if (!SlmenuCreateMultilineMenu(MenuContext.StringArray, MenuContext.StringCount, &Prompt, DisplayLineCount, &Index)) {
            SlmenuCleanupContext(&MenuContext);
            return EXIT_FAILURE;
        }

        if (Index < MenuContext.StringCount) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &MenuContext.StringArray[Index]);
        }

        SlmenuCleanupContext(&MenuContext);
    }
    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
