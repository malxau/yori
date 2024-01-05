/**
 * @file winpos/winpos.c
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
CHAR strWinPosHelpText[] =
        "\n"
        "Move or size application windows.\n"
        "\n"
        "WINPOS [-license] [-c|-t]\n"
        "WINPOS -a <title>\n"
        "WINPOS -m <title>\n"
        "WINPOS -n <title> <newtitle>\n"
        "WINPOS -p <title> <coordinates>\n"
        "WINPOS -r <title>\n"
        "WINPOS -s <title> <coordinates>\n"
        "WINPOS -x <title>\n"
        "\n"
        "   -a             Activate a specified window\n"
        "   -c             Cascade windows on the desktop\n"
        "   -m             Minimize a specified window\n"
        "   -n             Set the window title\n"
        "   -p             Position a specified window\n"
        "   -r             Restore a specified window (not minimized or maximized)\n"
        "   -s             Resize a specified window\n"
        "   -t             Tile windows on the desktop\n"
        "   -x             Maximize a specified window\n";

/**
 Display usage text to the user.
 */
BOOL
WinPosHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("WinPos %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strWinPosHelpText);
    return TRUE;
}


/**
 A set of operations that this program can perform.
 */
typedef enum _WINPOS_OPERATION {
    WinPosOperationNone = 0,
    WinPosOperationCascade = 1,
    WinPosOperationTile = 2,
    WinPosOperationMove = 3,
    WinPosOperationSize = 4,
    WinPosOperationMinimize = 5,
    WinPosOperationMaximize = 6,
    WinPosOperationRestore = 7,
    WinPosOperationActivate = 8,
    WinPosOperationName = 9,
} WINPOS_OPERATION;

/**
 Parse a string specifying x*y coordinates into their integer values.

 @param WindowCoordinates Pointer to the coordinates in string form.

 @param Horizontal On successful completion, updated to contain the
        horizontal coordinate.

 @param Vertical On successful completion, updated to contain the
        vertical coordinate.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
WinPosStringToCoordinates(
    __in PYORI_STRING WindowCoordinates,
    __out PLONG Horizontal,
    __out PLONG Vertical
    )
{
    YORI_MAX_SIGNED_T Number;
    YORI_STRING Remainder;
    YORI_ALLOC_SIZE_T CharsConsumed;

    if (!YoriLibStringToNumber(WindowCoordinates, TRUE, &Number, &CharsConsumed)) {
        return FALSE;
    }

    if (CharsConsumed == 0) {
        return FALSE;
    }

    if (WindowCoordinates->LengthInChars < (DWORD)(CharsConsumed + 2)) {
        return FALSE;
    }

    YoriLibInitEmptyString(&Remainder);
    Remainder.StartOfString = &WindowCoordinates->StartOfString[CharsConsumed];
    Remainder.LengthInChars = WindowCoordinates->LengthInChars - CharsConsumed;

    if (Remainder.StartOfString[0] != '*') {
        return FALSE;
    }

    Remainder.StartOfString++;
    Remainder.LengthInChars--;

    *Horizontal = (LONG)Number;

    if (!YoriLibStringToNumber(&Remainder, TRUE, &Number, &CharsConsumed)) {
        return FALSE;
    }

    if (CharsConsumed == 0) {
        return FALSE;
    }

    *Vertical = (LONG)Number;
    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the winpos builtin command.
 */
#define ENTRYPOINT YoriCmd_WINPOS
#else
/**
 The main entrypoint for the winpos standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the winpos cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ENTRYPOINT(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOLEAN ArgumentUnderstood;
    YORI_ALLOC_SIZE_T StartArg = 0;
    YORI_ALLOC_SIZE_T i;
    YORI_STRING Arg;
    PYORI_STRING WindowTitle = NULL;
    PYORI_STRING NewWindowTitle = NULL;
    PYORI_STRING WindowCoordinates = NULL;
    WINPOS_OPERATION Operation;

    Operation = WinPosOperationNone;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                WinPosHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("a")) == 0) {
                if (ArgC > i + 1) {
                    Operation = WinPosOperationActivate;
                    WindowTitle = &ArgV[i + 1];
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                Operation = WinPosOperationCascade;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("m")) == 0) {
                if (ArgC > i + 1) {
                    Operation = WinPosOperationMinimize;
                    WindowTitle = &ArgV[i + 1];
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("n")) == 0) {
                if (ArgC > i + 2) {
                    Operation = WinPosOperationName;
                    WindowTitle = &ArgV[i + 1];
                    NewWindowTitle = &ArgV[i + 2];
                    ArgumentUnderstood = TRUE;
                    i += 2;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("p")) == 0) {
                if (ArgC > i + 2) {
                    Operation = WinPosOperationMove;
                    WindowTitle = &ArgV[i + 1];
                    WindowCoordinates = &ArgV[i + 2];
                    ArgumentUnderstood = TRUE;
                    i += 2;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("r")) == 0) {
                if (ArgC > i + 1) {
                    Operation = WinPosOperationRestore;
                    WindowTitle = &ArgV[i + 1];
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                if (ArgC > i + 2) {
                    Operation = WinPosOperationSize;
                    WindowTitle = &ArgV[i + 1];
                    WindowCoordinates = &ArgV[i + 2];
                    ArgumentUnderstood = TRUE;
                    i += 2;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("t")) == 0) {
                Operation = WinPosOperationTile;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("x")) == 0) {
                if (ArgC > i + 1) {
                    Operation = WinPosOperationMaximize;
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

    if (Operation == WinPosOperationNone) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("winpos: missing operation\n"));
        return EXIT_FAILURE;
    }

    YoriLibLoadUser32Functions();

    switch(Operation) {
        case WinPosOperationCascade:
            if (DllUser32.pCascadeWindows == NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("winpos: operating system support not present\n"));
                return EXIT_FAILURE;
            } else {
                DllUser32.pCascadeWindows(NULL, 0, NULL, 0, NULL);
            }
            break;
        case WinPosOperationTile:
            if (DllUser32.pTileWindows == NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("winpos: operating system support not present\n"));
                return EXIT_FAILURE;
            } else {
                DllUser32.pTileWindows(NULL, 0, NULL, 0, NULL);
            }
            break;
        case WinPosOperationMove:
            if (DllUser32.pMoveWindow == NULL ||
                DllUser32.pFindWindowW == NULL ||
                DllUser32.pGetWindowRect == NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("winpos: operating system support not present\n"));
                return EXIT_FAILURE;
            } else {
                HWND Window;
                RECT CurrentPosition;
                LONG NewLeft;
                LONG NewTop;
                Window = DllUser32.pFindWindowW(NULL, WindowTitle->StartOfString);
                if (Window == NULL) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("winpos: matching window not found\n"));
                    return EXIT_FAILURE;
                }
                DllUser32.pGetWindowRect(Window, &CurrentPosition);
                if (!WinPosStringToCoordinates(WindowCoordinates, &NewLeft, &NewTop)) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("winpos: failed to parse coordinates\n"));
                    return EXIT_FAILURE;
                }
                CurrentPosition.bottom = CurrentPosition.bottom + NewTop - CurrentPosition.top;
                CurrentPosition.right = CurrentPosition.right + NewLeft - CurrentPosition.left;
                CurrentPosition.left = NewLeft;
                CurrentPosition.top = NewTop;
                DllUser32.pMoveWindow(Window, CurrentPosition.left, CurrentPosition.top, CurrentPosition.right - CurrentPosition.left, CurrentPosition.bottom - CurrentPosition.top, TRUE);

            }
            break;
        case WinPosOperationSize:
            if (DllUser32.pMoveWindow == NULL ||
                DllUser32.pFindWindowW == NULL ||
                DllUser32.pGetWindowRect == NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("winpos: operating system support not present\n"));
                return EXIT_FAILURE;
            } else {
                HWND Window;
                RECT CurrentPosition;
                LONG NewHeight;
                LONG NewWidth;
                Window = DllUser32.pFindWindowW(NULL, WindowTitle->StartOfString);
                if (Window == NULL) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("winpos: matching window not found\n"));
                    return EXIT_FAILURE;
                }
                DllUser32.pGetWindowRect(Window, &CurrentPosition);
                if (!WinPosStringToCoordinates(WindowCoordinates, &NewWidth, &NewHeight)) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("winpos: failed to parse coordinates\n"));
                    return EXIT_FAILURE;
                }
                DllUser32.pMoveWindow(Window, CurrentPosition.left, CurrentPosition.top, NewWidth, NewHeight, TRUE);
            }
            break;
        case WinPosOperationMinimize:
            if (DllUser32.pShowWindow == NULL ||
                DllUser32.pFindWindowW == NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("winpos: operating system support not present\n"));
                return EXIT_FAILURE;
            } else {
                HWND Window;
                Window = DllUser32.pFindWindowW(NULL, WindowTitle->StartOfString);
                if (Window == NULL) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("winpos: matching window not found\n"));
                    return EXIT_FAILURE;
                }
                DllUser32.pShowWindow(Window, SW_MINIMIZE);
            }
            break;
        case WinPosOperationMaximize:
            if (DllUser32.pShowWindow == NULL ||
                DllUser32.pFindWindowW == NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("winpos: operating system support not present\n"));
                return EXIT_FAILURE;
            } else {
                HWND Window;
                Window = DllUser32.pFindWindowW(NULL, WindowTitle->StartOfString);
                if (Window == NULL) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("winpos: matching window not found\n"));
                    return EXIT_FAILURE;
                }
                DllUser32.pShowWindow(Window, SW_MAXIMIZE);
            }
            break;
        case WinPosOperationRestore:
            if (DllUser32.pShowWindow == NULL ||
                DllUser32.pFindWindowW == NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("winpos: operating system support not present\n"));
                return EXIT_FAILURE;
            } else {
                HWND Window;
                Window = DllUser32.pFindWindowW(NULL, WindowTitle->StartOfString);
                if (Window == NULL) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("winpos: matching window not found\n"));
                    return EXIT_FAILURE;
                }
                DllUser32.pShowWindow(Window, SW_RESTORE);
            }
            break;
        case WinPosOperationActivate:
            if (DllUser32.pSetForegroundWindow == NULL ||
                DllUser32.pFindWindowW == NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("winpos: operating system support not present\n"));
                return EXIT_FAILURE;
            } else {
                HWND Window;
                Window = DllUser32.pFindWindowW(NULL, WindowTitle->StartOfString);
                if (Window == NULL) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("winpos: matching window not found\n"));
                    return EXIT_FAILURE;
                }
                DllUser32.pSetForegroundWindow(Window);
            }
            break;
        case WinPosOperationName:
            if (DllUser32.pSetWindowTextW == NULL ||
                DllUser32.pFindWindowW == NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("winpos: operating system support not present\n"));
                return EXIT_FAILURE;
            } else {
                HWND Window;
                Window = DllUser32.pFindWindowW(NULL, WindowTitle->StartOfString);
                if (Window == NULL) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("winpos: matching window not found\n"));
                    return EXIT_FAILURE;
                }
                DllUser32.pSetWindowTextW(Window, NewWindowTitle->StartOfString);
            }
            break;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
