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
 Parses a NULL terminated command line string into an argument count and array
 of YORI_STRINGs corresponding to arguments.

 @param szCmdLine The NULL terminated command line.

 @param argc On successful completion, populated with the count of arguments.

 @return A pointer to an array of YORI_STRINGs containing the parsed
         arguments.
 */
PYORI_STRING
YoriLibCmdlineToArgcArgv(LPTSTR szCmdLine, DWORD * argc)
{
    DWORD ArgCount = 0;
    DWORD CharCount = 0;
    TCHAR BreakChar = ' ';
    TCHAR * c;
    PYORI_STRING ArgvArray;
    LPTSTR ReturnStrings;

    //
    //  Consume all spaces.  After this, we're either at
    //  the end of string, or we have an arg, and it
    //  might start with a quote
    //

    c = szCmdLine;
    while (*c == ' ') c++;
    if (*c == '"') {
        BreakChar = '"';
        c++;
    }

    while (*c != '\0') {
        if (*c == BreakChar) {
            BreakChar = ' ';
            c++;
            while (*c == BreakChar) c++;
            if (*c == '"') {
                BreakChar = '"';
                c++;
            }
            ArgCount++;
        } else {
            CharCount++;

            //
            //  If we hit a break char, we count the argument then.
            //  If we hit end of string, count it here; note we're
            //  only counting it if we counted a character before it
            //  (ie., trailing whitespace is not an arg.)
            //

            c++;

            if (*c == '\0') {
                ArgCount++;
            }
        }
    }

    *argc = ArgCount;
    if (ArgCount == 0) {
        return NULL;
    }

    ArgvArray = YoriLibReferencedMalloc( (ArgCount * sizeof(YORI_STRING)) + (CharCount + ArgCount) * sizeof(TCHAR));
    if (ArgvArray == NULL) {
        *argc = 0;
        return NULL;
    }

    ReturnStrings = (LPTSTR)(ArgvArray + ArgCount);

    ArgCount = 0;
    YoriLibInitEmptyString(&ArgvArray[ArgCount]);
    ArgvArray[ArgCount].StartOfString = ReturnStrings;
    ArgvArray[ArgCount].MemoryToFree = ArgvArray;
    YoriLibReference(ArgvArray);

    //
    //  Consume all spaces.  After this, we're either at
    //  the end of string, or we have an arg, and it
    //  might start with a quote
    //

    c = szCmdLine;
    while (*c == ' ') c++;
    BreakChar = ' ';
    if (*c == '"') {
        BreakChar = '"';
        c++;
    }

    while (*c != '\0') {
        if (*c == BreakChar) {
            *ReturnStrings = '\0';
            ReturnStrings++;
            ArgvArray[ArgCount].LengthAllocated = ArgvArray[ArgCount].LengthInChars + 1;

            BreakChar = ' ';
            c++;
            while (*c == BreakChar) c++;
            if (*c == '"') {
                BreakChar = '"';
                c++;
            }
            if (*c != '\0') {
                ArgCount++;
                YoriLibInitEmptyString(&ArgvArray[ArgCount]);
                ArgvArray[ArgCount].StartOfString = ReturnStrings;
                YoriLibReference(ArgvArray);
                ArgvArray[ArgCount].MemoryToFree = ArgvArray;
            }
        } else {
            *ReturnStrings = *c;
            ReturnStrings++;
            ArgvArray[ArgCount].LengthInChars++;

            //
            //  If we hit a break char, we count the argument then.
            //  If we hit end of string, count it here; note we're
            //  only counting it if we counted a character before it
            //  (ie., trailing whitespace is not an arg.)
            //

            c++;

            if (*c == '\0') {
                *ReturnStrings = '\0';
                ArgvArray[ArgCount].LengthAllocated = ArgvArray[ArgCount].LengthInChars + 1;
            }
        }
    }

    return ArgvArray;
}


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
DWORD CONSOLE_USER_ENTRYPOINT(DWORD ArgC, YORI_STRING ArgV[]);

/**
 The entrypoint function that the Windows loader will commence execution from.
 */
VOID __cdecl CONSOLE_CRT_ENTRYPOINT()
{
    PYORI_STRING ArgV;
    DWORD ArgC;
    DWORD Index;
    DWORD ExitCode;

    YoriLibLoadNtDllFunctions();
    YoriLibLoadKernel32Functions();

    ArgV = YoriLibCmdlineToArgcArgv(GetCommandLine(), &ArgC);
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
