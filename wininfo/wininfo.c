/**
 * @file wininfo/wininfo.c
 *
 * Yori shell reposition or resize windows
 *
 * Copyright (c) 2018 Malcolm J. Smith
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

/**
 Help text to display to the user.
 */
const
CHAR strWinInfoHelpText[] =
        "\n"
        "Return information about a window.\n"
        "\n"
        "WININFO [-license] [-f <fmt>] [-t <title>]\n"
        "\n"
        "Format specifiers are:\n"
        "   $left$         The offset from the left of the screen to the window\n"
        "   $top$          The offset from the top of the screen to the window\n"
        "   $width$        The width of the window\n"
        "   $height$       The height of the window\n";

/**
 Display usage text to the user.
 */
BOOL
WinInfoHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("WinInfo %i.%02i\n"), WININFO_VER_MAJOR, WININFO_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strWinInfoHelpText);
    return TRUE;
}

/**
 Context information used when populating the output string.
 */
typedef struct _WININFO_CONTEXT {

    /**
     Handle to the window to return information for.
     */
    HWND Window;

    /**
     The coordinates of the window.
     */
    RECT WindowRect;
} WININFO_CONTEXT, *PWININFO_CONTEXT;

/**
 A callback function to expand any known variables found when parsing the
 format string.

 @param OutputString A pointer to the output string to populate with data
        if a known variable is found.  The length allocated contains the
        length that can be populated with data.

 @param VariableName The variable name to expand.

 @param Context Pointer to a @ref WININFO_CONTEXT structure containing
        the data to populate.
 
 @return The number of characters successfully populated, or the number of
         characters required in order to successfully populate, or zero
         on error.
 */
DWORD
WinInfoExpandVariables(
    __inout PYORI_STRING OutputString,
    __in PYORI_STRING VariableName,
    __in PVOID Context
    )
{
    DWORD CharsNeeded;
    PWININFO_CONTEXT WinInfoContext = (PWININFO_CONTEXT)Context;

    if (YoriLibCompareStringWithLiteral(VariableName, _T("left")) == 0) {
        CharsNeeded = YoriLibSPrintfSize(_T("%i"), WinInfoContext->WindowRect.left);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("top")) == 0) {
        CharsNeeded = YoriLibSPrintfSize(_T("%i"), WinInfoContext->WindowRect.top);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("width")) == 0) {
        CharsNeeded = YoriLibSPrintfSize(_T("%i"), WinInfoContext->WindowRect.right - WinInfoContext->WindowRect.left);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("height")) == 0) {
        CharsNeeded = YoriLibSPrintfSize(_T("%i"), WinInfoContext->WindowRect.bottom - WinInfoContext->WindowRect.top);
    } else {
        return 0;
    }

    if (OutputString->LengthAllocated < CharsNeeded) {
        return CharsNeeded;
    }

    if (YoriLibCompareStringWithLiteral(VariableName, _T("left")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputString->StartOfString, _T("%i"), WinInfoContext->WindowRect.left);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("top")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputString->StartOfString, _T("%i"), WinInfoContext->WindowRect.top);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("width")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputString->StartOfString, _T("%i"), WinInfoContext->WindowRect.right - WinInfoContext->WindowRect.left);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("height")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputString->StartOfString, _T("%i"), WinInfoContext->WindowRect.bottom - WinInfoContext->WindowRect.top);
    }

    OutputString->LengthInChars = CharsNeeded;
    return CharsNeeded;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the wininfo builtin command.
 */
#define ENTRYPOINT YoriCmd_WININFO
#else
/**
 The main entrypoint for the wininfo standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the wininfo cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD StartArg = 0;
    DWORD i;
    YORI_STRING Arg;
    PYORI_STRING WindowTitle = NULL;
    YORI_STRING DisplayString;
    WININFO_CONTEXT Context;
    LPTSTR FormatString = _T("Position: $left$*$top$\n")
                          _T("Size:     $width$*$height$\n");
    YORI_STRING YsFormatString;

    YoriLibInitEmptyString(&YsFormatString);
    ZeroMemory(&Context, sizeof(Context));

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                WinInfoHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("f")) == 0) {
                if (ArgC >= i + 1) {
                    YsFormatString.StartOfString = ArgV[i + 1].StartOfString;
                    YsFormatString.LengthInChars = ArgV[i + 1].LengthInChars;
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("t")) == 0) {
                if (ArgC >= i + 1) {
                    WindowTitle = &ArgV[i + 1];
                    ArgumentUnderstood = TRUE;
                    i++;
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

    YoriLibLoadUser32Functions();

    if (WindowTitle != NULL) {
        if (DllUser32.pFindWindowW == NULL) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("wininfo: operating system support not present\n"));
            return EXIT_FAILURE;
        }

        Context.Window = DllUser32.pFindWindowW(NULL, WindowTitle->StartOfString);
        if (Context.Window == NULL) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("wininfo: window not found\n"));
            return EXIT_FAILURE;
        }
        if (DllUser32.pGetWindowRect != NULL) {
            DllUser32.pGetWindowRect(Context.Window, &Context.WindowRect);
        }
    } else {
        if (DllUser32.pGetDesktopWindow == NULL) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("wininfo: operating system support not present\n"));
            return EXIT_FAILURE;
        }

        Context.Window = DllUser32.pGetDesktopWindow();
        if (DllUser32.pGetClientRect != NULL) {
            DllUser32.pGetClientRect(Context.Window, &Context.WindowRect);
        }
    }


    YoriLibInitEmptyString(&DisplayString);
    if (YsFormatString.StartOfString == NULL) {
        YoriLibConstantString(&YsFormatString, FormatString);
    }
    YoriLibExpandCommandVariables(&YsFormatString, '$', FALSE, WinInfoExpandVariables, &Context, &DisplayString);
    if (DisplayString.StartOfString != NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
        YoriLibFreeStringContents(&DisplayString);
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
