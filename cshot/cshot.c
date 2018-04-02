/**
 * @file cshot/cshot.c
 *
 * Yori shell cshot
 *
 * Copyright (c) 2017 Malcolm J. Smith
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
 * FITNESS CSHOT A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE CSHOT ANY CLAIM, DAMAGES OR OTHER
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
CHAR strHelpText[] =
        "\n"
        "Captures previous output on the console and outputs to standard output.\n"
        "\n"
        "CSHOT [-s num] [-c num]\n"
        ;

/**
 Display usage text to the user.
 */
BOOL
CshotHelp()
{
    YORI_STRING License;

    YoriLibMitLicenseText(_T("2017"), &License);

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Cshot %i.%i\n"), CSHOT_VER_MAJOR, CSHOT_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs\n"), strHelpText);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &License);
    YoriLibFreeStringContents(&License);
    return TRUE;
}

/**
 The main entrypoint for the cshot cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ymain(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD SkipCount = 0;
    DWORD LineCount = 0;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    HANDLE hConsole;
    SMALL_RECT ReadWindow;
    PCHAR_INFO ReadBuffer;
    COORD ReadBufferSize;
    COORD ReadBufferOffset;
    DWORD LastAttribute;
    WORD LineIndex;
    DWORD i;
    YORI_STRING Arg;
    LONGLONG Temp;
    DWORD CharsConsumed;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                CshotHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                if (ArgC > i + 1) {
                    if (YoriLibStringToNumber(&ArgV[i + 1], TRUE, &Temp, &CharsConsumed)) {


                        LineCount = (DWORD)Temp;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                if (ArgC > i + 1) {
                    if (YoriLibStringToNumber(&ArgV[i + 1], TRUE, &Temp, &CharsConsumed)) {
                        SkipCount = (DWORD)Temp;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            }
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    hConsole = CreateFile(_T("CONOUT$"), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (hConsole == INVALID_HANDLE_VALUE) {
        return EXIT_FAILURE;
    }

    if (!GetConsoleScreenBufferInfo(hConsole, &ScreenInfo)) {
        CloseHandle(hConsole);
        return EXIT_FAILURE;
    }

    if (LineCount == 0) {
        LineCount = ScreenInfo.dwSize.Y - ScreenInfo.dwCursorPosition.Y;
    } else if (LineCount > (DWORD)ScreenInfo.dwCursorPosition.Y) {
        LineCount = ScreenInfo.dwCursorPosition.Y;
    }

    if (SkipCount >= (DWORD)ScreenInfo.dwCursorPosition.Y) {
        CloseHandle(hConsole);
        return EXIT_FAILURE;
    }

    if (LineCount + SkipCount > (DWORD)ScreenInfo.dwCursorPosition.Y) {
        LineCount = ScreenInfo.dwCursorPosition.Y - SkipCount;
    }

    if (LineCount == 0) {
        CloseHandle(hConsole);
        return EXIT_FAILURE;
    }

    ReadWindow.Left = 0;
    ReadWindow.Right = (WORD)(ScreenInfo.dwSize.X - 1);
    ReadWindow.Top = (WORD)(ScreenInfo.dwCursorPosition.Y - SkipCount - LineCount);
    ReadWindow.Bottom = (WORD)(ScreenInfo.dwCursorPosition.Y - SkipCount - 1);

    ReadBufferSize.X = (WORD)(ReadWindow.Right - ReadWindow.Left + 1);
    ReadBufferSize.Y = (WORD)(ReadWindow.Bottom - ReadWindow.Top + 1);

    ReadBuffer = YoriLibMalloc(ReadBufferSize.X * ReadBufferSize.Y * sizeof(CHAR_INFO));
    if (ReadBuffer == NULL) {
        CloseHandle(hConsole);
        return EXIT_FAILURE;
    }

    ReadBufferOffset.X = 0;
    ReadBufferOffset.Y = 0;

    //
    //  ReadConsoleOutput fails if it's given a large request, so give it
    //  a pile of small (one line) requests.
    //

    for (LineIndex = 0; LineIndex < (WORD)ReadBufferSize.Y; LineIndex++) {
        SMALL_RECT LineReadWindow;
        COORD LineReadBufferSize;

        LineReadWindow.Left = ReadWindow.Left;
        LineReadWindow.Right = ReadWindow.Right;
        LineReadWindow.Top = (WORD)(ReadWindow.Top + LineIndex);
        LineReadWindow.Bottom = LineReadWindow.Top;

        LineReadBufferSize.X = ReadBufferSize.X;
        LineReadBufferSize.Y = 1;

        if (!ReadConsoleOutput(hConsole, &ReadBuffer[LineIndex * ReadBufferSize.X], LineReadBufferSize, ReadBufferOffset, &LineReadWindow)) {
            DWORD Err = GetLastError();
            LPTSTR ErrText = YoriLibGetWinErrorText(Err);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("cshot: ReadConsoleOutput failed: %s"), ErrText);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ReadBufferSize %ix%i ReadWindow (%i,%i)-(%i,%i)\n"), ReadBufferSize.X, ReadBufferSize.Y, ReadWindow.Left, ReadWindow.Top, ReadWindow.Right, ReadWindow.Bottom);
            YoriLibFreeWinErrorText(ErrText);
            CloseHandle(hConsole);
            YoriLibFree(ReadBuffer);
            return EXIT_FAILURE;
        }
    }

    YoriLibVtSetConsoleTextAttribute(YORI_LIB_OUTPUT_STDOUT, ReadBuffer[0].Attributes);
    LastAttribute = ReadBuffer[0].Attributes;

    for (ReadBufferOffset.Y = 0; ReadBufferOffset.Y < ReadBufferSize.Y; ReadBufferOffset.Y++) {
        DWORD CurrentMode;

        for (ReadBufferOffset.X = 0; ReadBufferOffset.X < ReadBufferSize.X; ReadBufferOffset.X++) {
            DWORD CellIndex;

            CellIndex = ReadBufferOffset.Y * ReadBufferSize.X + ReadBufferOffset.X;
            
            if (ReadBuffer[CellIndex].Attributes != LastAttribute) {
                YoriLibVtSetConsoleTextAttribute(YORI_LIB_OUTPUT_STDOUT, ReadBuffer[CellIndex].Attributes);
                LastAttribute = ReadBuffer[CellIndex].Attributes;
            }
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%c"), ReadBuffer[CellIndex].Char.UnicodeChar);
        }
        if (!GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &CurrentMode)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
        }
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
