/**
 * @file contool/contool.c
 *
 * Yori shell configure console or display console properties
 *
 * Copyright (c) 2017-2023 Malcolm J. Smith
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
CHAR strConToolHelpText[] =
        "\n"
        "Configures console properties or outputs console information in a specified format.\n"
        "\n"
        "CONTOOL [-license] [-fullscreen|-window] [-f <fmt>]\n"
        "\n"
        "   -fullscreen    Switch to full screen\n"
        "   -window        Switch to a console window\n"
        "\n"
        "Format specifiers are:\n"
        "   $buffer_x$           The width of the scrollback buffer in cells\n"
        "   $buffer_y$           The height of the scrollback buffer in cells\n"
        "   $font$               The console font name\n"
        "   $font_weight$        Font weight (400 = normal, 700 = bold)\n"
        "   $font_x$             The width of each cell\n"
        "   $font_y$             The height of each cell\n"
        "   $window_x$           The width of the window in cells\n"
        "   $window_y$           The height of the window in cells\n" ;

/**
 Display usage text to the user.
 */
BOOL
ConToolHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("ConTool %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strConToolHelpText);
    return TRUE;
}

/**
 A context structure to pass to the function expanding variables so it knows
 what values to use.
 */
typedef struct _CONTOOL_RESULT {

    /**
     Set to TRUE if a variable was specified that could not be expanded or
     if a modification operation failed.  This may indicate either a caller
     error in asking for an unknown variable, or asking for a variable which
     wasn't available.  When this occurs, the process exits with a failure
     code.
     */
    BOOLEAN Failure;


    /**
     Flags indicating which data has been collected.
     */
    union {
        DWORD All;
        struct {
            BOOLEAN ScreenBufferInfo:1;
            BOOLEAN FontInfo:1;
            BOOLEAN FullScreenInfo:1;
        };
    } Have;

    /**
     TRUE if the console is in full screen mode.
     */
    BOOLEAN FullScreen;

    /**
     Properties including window size, buffer size, cursor position, and
     active color.
     */
    CONSOLE_SCREEN_BUFFER_INFO ScreenBufferInfo;

    /**
     Properties including font and size.
     */
    YORI_CONSOLE_FONT_INFOEX FontInfo;

} CONTOOL_RESULT, *PCONTOOL_RESULT;

/**
 A callback function to expand any known variables found when parsing the
 format string.

 @param OutputString A pointer to the output string to populate with data
        if a known variable is found.  The length allocated contains the
        length that can be populated with data.

 @param VariableName The variable name to expand.

 @param Context Pointer to a @ref CONTOOL_RESULT structure containing
        the data to populate.
 
 @return The number of characters successfully populated, or the number of
         characters required in order to successfully populate, or zero
         on error.
 */
DWORD
ConToolExpandVariables(
    __inout PYORI_STRING OutputString,
    __in PYORI_STRING VariableName,
    __in PVOID Context
    )
{
    DWORD CharsNeeded;
    PCONTOOL_RESULT ConToolContext = (PCONTOOL_RESULT)Context;

    if (ConToolContext->Have.ScreenBufferInfo &&
        YoriLibCompareStringWithLiteral(VariableName, _T("buffer_x")) == 0) {

        CharsNeeded = YoriLibSPrintfSize(_T("%i"), ConToolContext->ScreenBufferInfo.dwSize.X);
    } else if (ConToolContext->Have.ScreenBufferInfo &&
               YoriLibCompareStringWithLiteral(VariableName, _T("buffer_y")) == 0) {

        CharsNeeded = YoriLibSPrintfSize(_T("%i"), ConToolContext->ScreenBufferInfo.dwSize.Y);

    } else if (ConToolContext->Have.FontInfo &&
               YoriLibCompareStringWithLiteral(VariableName, _T("font")) == 0) {

        CharsNeeded = YoriLibSPrintfSize(_T("%s"), ConToolContext->FontInfo.FaceName);

    } else if (ConToolContext->Have.FontInfo &&
               YoriLibCompareStringWithLiteral(VariableName, _T("font_weight")) == 0) {

        CharsNeeded = YoriLibSPrintfSize(_T("%i"), ConToolContext->FontInfo.FontWeight);

    } else if (ConToolContext->Have.FontInfo &&
               YoriLibCompareStringWithLiteral(VariableName, _T("font_x")) == 0) {

        CharsNeeded = YoriLibSPrintfSize(_T("%i"), ConToolContext->FontInfo.dwFontSize.X);

    } else if (ConToolContext->Have.FontInfo &&
               YoriLibCompareStringWithLiteral(VariableName, _T("font_y")) == 0) {

        CharsNeeded = YoriLibSPrintfSize(_T("%i"), ConToolContext->FontInfo.dwFontSize.Y);

    } else if (ConToolContext->Have.FullScreenInfo &&
               YoriLibCompareStringWithLiteral(VariableName, _T("fullscreen")) == 0) {

        CharsNeeded = YoriLibSPrintfSize(_T("%i"), ConToolContext->FullScreen?1:0);

    } else if (ConToolContext->Have.ScreenBufferInfo &&
               YoriLibCompareStringWithLiteral(VariableName, _T("window_x")) == 0) {

        CharsNeeded = YoriLibSPrintfSize(_T("%i"), ConToolContext->ScreenBufferInfo.srWindow.Right - ConToolContext->ScreenBufferInfo.srWindow.Left + 1);

    } else if (ConToolContext->Have.ScreenBufferInfo &&
               YoriLibCompareStringWithLiteral(VariableName, _T("window_y")) == 0) {

        CharsNeeded = YoriLibSPrintfSize(_T("%i"), ConToolContext->ScreenBufferInfo.srWindow.Bottom - ConToolContext->ScreenBufferInfo.srWindow.Top + 1);

    } else {
        ConToolContext->Failure = TRUE;
        return 0;
    }

    if (OutputString->LengthAllocated < CharsNeeded) {
        return CharsNeeded;
    }

    if (YoriLibCompareStringWithLiteral(VariableName, _T("buffer_x")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputString->StartOfString, _T("%i"), ConToolContext->ScreenBufferInfo.dwSize.X);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("buffer_y")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputString->StartOfString, _T("%i"), ConToolContext->ScreenBufferInfo.dwSize.Y);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("font")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputString->StartOfString, _T("%s"), ConToolContext->FontInfo.FaceName);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("font_weight")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputString->StartOfString, _T("%i"), ConToolContext->FontInfo.FontWeight);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("font_x")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputString->StartOfString, _T("%i"), ConToolContext->FontInfo.dwFontSize.X);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("font_y")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputString->StartOfString, _T("%i"), ConToolContext->FontInfo.dwFontSize.Y);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("fullscreen")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputString->StartOfString, _T("%i"), ConToolContext->FullScreen?1:0);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("window_x")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputString->StartOfString, _T("%i"), ConToolContext->ScreenBufferInfo.srWindow.Right - ConToolContext->ScreenBufferInfo.srWindow.Left + 1);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("window_y")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputString->StartOfString, _T("%i"), ConToolContext->ScreenBufferInfo.srWindow.Bottom - ConToolContext->ScreenBufferInfo.srWindow.Top + 1);
    }

    OutputString->LengthInChars = CharsNeeded;
    return CharsNeeded;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the contool builtin command.
 */
#define ENTRYPOINT YoriCmd_CONTOOL
#else
/**
 The main entrypoint for the contool standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the contool cmdlet.

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
    CONTOOL_RESULT ConToolResult;
    BOOL ArgumentUnderstood;
    YORI_STRING DisplayString;
    YORI_STRING YsFormatString;
    DWORD StartArg = 0;
    DWORD i;
    YORI_STRING Arg;
    HANDLE hConsole;
    BOOLEAN Fullscreen;
    BOOLEAN FullscreenSet;
    DWORD LastError;
    LPTSTR ErrText;

    Fullscreen = FALSE;
    FullscreenSet = FALSE;

    YoriLibInitEmptyString(&YsFormatString);

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                ConToolHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2023"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("f")) == 0) {
                if (ArgC > i + 1) {
                    YsFormatString.StartOfString = ArgV[i + 1].StartOfString;
                    YsFormatString.LengthInChars = ArgV[i + 1].LengthInChars;
                    YsFormatString.LengthAllocated = ArgV[i + 1].LengthAllocated;
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("fullscreen")) == 0) {
                Fullscreen = TRUE;
                FullscreenSet = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("window")) == 0) {
                Fullscreen = FALSE;
                FullscreenSet = TRUE;
                ArgumentUnderstood = TRUE;
            }
        } else {
            ArgumentUnderstood = TRUE;
            StartArg = i;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    ZeroMemory(&ConToolResult, sizeof(ConToolResult));

    hConsole = CreateFile(_T("CONOUT$"), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (hConsole == INVALID_HANDLE_VALUE) {
        return EXIT_FAILURE;
    }

    if (FullscreenSet) {

        if (DllKernel32.pSetConsoleDisplayMode != NULL) {
            DWORD DisplayMode;
            COORD NewSize;

            if (Fullscreen) {
                DisplayMode = CONSOLE_FULLSCREEN_MODE;
            } else {
                DisplayMode = CONSOLE_WINDOWED_MODE;
            }

            if (!DllKernel32.pSetConsoleDisplayMode(hConsole, DisplayMode, &NewSize)) {
                LastError = GetLastError();
                ErrText = YoriLibGetWinErrorText(LastError);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Changing console display mode failed: %s"), ErrText);
                YoriLibFreeWinErrorText(ErrText);
                ConToolResult.Failure = TRUE;
            }
        }
        CloseHandle(hConsole);
    } else {

        if (GetConsoleScreenBufferInfo(hConsole, &ConToolResult.ScreenBufferInfo)) {
            ConToolResult.Have.ScreenBufferInfo = TRUE;
        }

        if (DllKernel32.pGetConsoleDisplayMode != NULL) {
            DWORD ConsoleDisplayMode;
            if (DllKernel32.pGetConsoleDisplayMode(&ConsoleDisplayMode)) {
                if (ConsoleDisplayMode & CONSOLE_FULLSCREEN) {
                    ConToolResult.FullScreen = TRUE;
                }
                ConToolResult.Have.FullScreenInfo = TRUE;
            }
        }
    
        if (DllKernel32.pGetCurrentConsoleFontEx != NULL) {
            ZeroMemory(&ConToolResult.FontInfo, sizeof(ConToolResult.FontInfo));
            ConToolResult.FontInfo.cbSize = sizeof(ConToolResult.FontInfo);
            if (DllKernel32.pGetCurrentConsoleFontEx(hConsole, FALSE, &ConToolResult.FontInfo)) {
                ConToolResult.Have.FontInfo = TRUE;
            }
        }
    
        CloseHandle(hConsole);
    
        YoriLibInitEmptyString(&DisplayString);
    
        //
        //  If the user specified a string, use it.  If not, fall back to a
        //  series of defaults depending on the information we have collected.
        //
    
        if (YsFormatString.StartOfString != NULL) {
            YoriLibExpandCommandVariables(&YsFormatString, '$', FALSE, ConToolExpandVariables, &ConToolResult, &DisplayString);
            if (DisplayString.StartOfString != NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
                YoriLibFreeStringContents(&DisplayString);
            }
        } else {
    
            if (ConToolResult.Have.ScreenBufferInfo) {
    
                LPTSTR FormatString = 
                              _T("Buffer width:         $buffer_x$\n")
                              _T("Buffer height:        $buffer_y$\n")
                              _T("Window width:         $window_x$\n")
                              _T("Window height:        $window_y$\n");
                YoriLibConstantString(&YsFormatString, FormatString);
                YoriLibExpandCommandVariables(&YsFormatString, '$', FALSE, ConToolExpandVariables, &ConToolResult, &DisplayString);
                if (DisplayString.StartOfString != NULL) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
                }
            }
    
            if (ConToolResult.Have.FontInfo) {
    
                LPTSTR FormatString = 
                              _T("Font width:           $font_x$\n")
                              _T("Font height:          $font_y$\n")
                              _T("Font name:            $font$\n")
                              _T("Font weight:          $font_weight$\n");
                YoriLibConstantString(&YsFormatString, FormatString);
                YoriLibExpandCommandVariables(&YsFormatString, '$', FALSE, ConToolExpandVariables, &ConToolResult, &DisplayString);
                if (DisplayString.StartOfString != NULL) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
                }
            }

            if (ConToolResult.Have.FullScreenInfo) {
                LPTSTR FormatString = 
                              _T("Full screen:          $fullscreen$\n");
                YoriLibConstantString(&YsFormatString, FormatString);
                YoriLibExpandCommandVariables(&YsFormatString, '$', FALSE, ConToolExpandVariables, &ConToolResult, &DisplayString);
                if (DisplayString.StartOfString != NULL) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
                }
            }
    
            YoriLibFreeStringContents(&DisplayString);
        }
    }
    
    if (ConToolResult.Failure) {
        return EXIT_FAILURE;
    } else {
        return EXIT_SUCCESS;
    }
}

// vim:sw=4:ts=4:et:
