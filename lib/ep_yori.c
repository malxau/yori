/**
 * @file lib/ep_yori.c
 *
 * Entrypoint code for YoriLib applications
 *
 * Copyright (c) 2014-2018 Malcolm J. Smith
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

#include "yoripch.h"
#include "yorilib.h"

/**
 Specifies the main entrypoint function for the application after arguments
 have been parsed.
 */
#define CONSOLE_USER_ENTRYPOINT ymain

/**
 Specifies the entrypoint function that Windows will invoke.
 */
#define CONSOLE_CRT_ENTRYPOINT  ymainCRTStartup

/**
 The entrypoint function that YoriLib based applications commence execution
 from.
 */
DWORD CONSOLE_USER_ENTRYPOINT(YORI_ALLOC_SIZE_T ArgC, YORI_STRING ArgV[]);

/**
 The entrypoint function that the Windows loader will commence execution from.
 */
VOID __cdecl CONSOLE_CRT_ENTRYPOINT(VOID)
{
    PYORI_STRING ArgV;
    YORI_ALLOC_SIZE_T ArgC;
    YORI_ALLOC_SIZE_T Index;
    DWORD ExitCode;

    YoriLibLoadNtDllFunctions();
    YoriLibLoadKernel32Functions();

    ArgV = YoriLibCmdlineToArgcArgv(GetCommandLine(), YORI_MAX_ALLOC_SIZE, FALSE, &ArgC);
    if (ArgV == NULL) {
        ExitProcess(EXIT_FAILURE);
    }
    ExitCode = CONSOLE_USER_ENTRYPOINT(ArgC, ArgV);
    for (Index = 0; Index < ArgC; Index++) {
        YoriLibFreeStringContents(&ArgV[Index]);
    }
    YoriLibDereference(ArgV);

    YoriLibDisplayMemoryUsage();

    ExitProcess(ExitCode);
}


// vim:sw=4:ts=4:et:
