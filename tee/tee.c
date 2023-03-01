/**
 * @file tee/tee.c
 *
 * Yori shell output to a file and stdout
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
CHAR strTeeHelpText[] =
        "\n"
        "Output the contents of standard input to standard output and a file.\n"
        "\n"
        "TEE [-license] -c\n"
        "TEE [-license] [-a] <file>]\n"
        "\n"
        "   -a             Append to the file\n"
        "   -c             Write to the console and standard output\n";

/**
 Display usage text to the user.
 */
BOOL
TeeHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Tee %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strTeeHelpText);
    return TRUE;
}

/**
 Context passed to the callback which is invoked for each source stream
 processed.
 */
typedef struct _TEE_CONTEXT {

    /**
     Handle to a file which will receive all output in addition to standard
     output.
     */
    HANDLE hFile;

    /**
     TRUE if hFile is a handle to a console; FALSE if it is a handle to a
     different type of device.
     */
    BOOLEAN FileIsConsole;

} TEE_CONTEXT, *PTEE_CONTEXT;

/**
 Write a single line to an output device.

 @param hDevice Handle to the output device.

 @param IsConsole TRUE if the output device is a console; FALSE if it is not.
        This is used to determine whether a newline needs to be inserted in
        cases where the console has advanced to the next line automatically.

 @param Line Pointer to the line to output.
 */
VOID
TeeWriteLine(
    __in HANDLE hDevice,
    __in BOOLEAN IsConsole,
    __in PYORI_STRING Line
    )
{
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;

    //
    //  If it's not a console, output the line and newline in one operation.
    //  If it is a console, output the line, and see where the cursor is,
    //  then optionally include a newline.
    //

    if (!IsConsole) {
        YoriLibOutputToDevice(hDevice, 0, _T("%y\n"), Line);
    } else {
        YoriLibOutputToDevice(hDevice, 0, _T("%y"), Line);
        if (Line->LengthInChars == 0 ||
            !GetConsoleScreenBufferInfo(hDevice, &ScreenInfo) ||
            ScreenInfo.dwCursorPosition.X != 0) {

            YoriLibOutputToDevice(hDevice, 0, _T("\n"));
        }
    }
}

/**
 Process a single stream.

 @param hSource Handle to the source.

 @param TeeContext Pointer to the context for the operation, including a
        handle to the output stream to write data to.

 @return TRUE to indicate success or FALSE to indicate failure.
 */
BOOL
TeeProcessStream(
    __in HANDLE hSource,
    __in PTEE_CONTEXT TeeContext
    )
{
    PVOID LineContext = NULL;
    DWORD Junk;
    YORI_STRING LineString;
    HANDLE StdOutHandle;
    BOOLEAN StdOutIsConsole;

    StdOutIsConsole = FALSE;

    StdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleMode(StdOutHandle, &Junk)) {
        StdOutIsConsole = TRUE;
    }

    YoriLibInitEmptyString(&LineString);

    while (TRUE) {

        if (!YoriLibReadLineToString(&LineString, &LineContext, hSource)) {
            break;
        }

        TeeWriteLine(StdOutHandle, StdOutIsConsole, &LineString);
        TeeWriteLine(TeeContext->hFile, TeeContext->FileIsConsole, &LineString);
    }

    YoriLibLineReadCloseOrCache(LineContext);
    YoriLibFreeStringContents(&LineString);

    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the tee builtin command.
 */
#define ENTRYPOINT YoriCmd_TEE
#else
/**
 The main entrypoint for the tee standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the tee cmdlet.

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
    DWORD i;
    DWORD StartArg = 0;
    DWORD DesiredAccess;
    BOOLEAN Append = FALSE;
    BOOLEAN Console = FALSE;
    TEE_CONTEXT TeeContext;
    YORI_STRING FileName;
    YORI_STRING Arg;

    ZeroMemory(&TeeContext, sizeof(TeeContext));

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                TeeHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2023"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("a")) == 0) {
                Append = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                Console = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("-")) == 0) {
                StartArg = i + 1;
                ArgumentUnderstood = TRUE;
                break;
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

    //
    //  If no file name is specified, use stdin; otherwise open
    //  the file and use that
    //

    if (!Console && (StartArg == 0 || StartArg == ArgC)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("tee: argument missing\n"));
        return EXIT_FAILURE;
    }

    if (YoriLibIsStdInConsole()) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("tee: No file or pipe for input\n"));
        return EXIT_FAILURE;
    }

    if (Console) {
        YoriLibConstantString(&FileName, _T("CONOUT$"));
        TeeContext.FileIsConsole = TRUE;

        //
        //  Open for read and write so we can query the cursor location.
        //

        DesiredAccess = GENERIC_READ | GENERIC_WRITE;
    } else {

        if (!YoriLibUserStringToSingleFilePath(&ArgV[StartArg], TRUE, &FileName)) {
            DWORD LastError = GetLastError();
            LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("tee: getfullpathname of %y failed: %s"), &ArgV[StartArg], ErrText);
            YoriLibFreeWinErrorText(ErrText);
            return EXIT_FAILURE;
        }
        DesiredAccess = (Append?FILE_APPEND_DATA:FILE_WRITE_DATA) | SYNCHRONIZE;
    }

    TeeContext.hFile = CreateFile(FileName.StartOfString,
                                  DesiredAccess,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                  NULL,
                                  OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);

    if (TeeContext.hFile == INVALID_HANDLE_VALUE || TeeContext.hFile == NULL) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("tee: open of %y failed: %s"), &FileName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&FileName);
        return EXIT_FAILURE;
    }

    TeeProcessStream(GetStdHandle(STD_INPUT_HANDLE), &TeeContext);

#if !YORI_BUILTIN
    YoriLibLineReadCleanupCache();
#endif

    CloseHandle(TeeContext.hFile);
    YoriLibFreeStringContents(&FileName);

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
