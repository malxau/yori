/**
 * @file crt/ep_cons.c
 *
 * Entrypoint code for console applications
 *
 * Copyright (c) 2014 Malcolm J. Smith
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

#ifndef WIN32_LEAN_AND_MEAN
/**
 Standard define to include somewhat less of the Windows headers.  Command
 line software doesn't need much.
 */
#define WIN32_LEAN_AND_MEAN 1
#endif

#pragma warning(disable: 4001) /* Single line comment */
#pragma warning(disable: 4127) // conditional expression constant
#pragma warning(disable: 4201) // nameless struct/union
#pragma warning(disable: 4214) // bit field type other than int
#pragma warning(disable: 4514) // unreferenced inline function

#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(disable: 26451) // Arithmetic overflow
#if (_MSC_VER >= 1939)
//
//  These warnings aim to check for integer overflows when allocating memory,
//  resulting in a smaller allocation than expected.  Unfortunately the
//  implementation is quite bad:
//
//   - An allocator is a function with "alloc" in its name, which matches
//     Yori's bounds check functions
//   - There is no annotation or awareness of a bounds check function, so it
//     will catch overflows that have already been checked.
//
//  The "best" thing Yori could do here is implement bounds check functions,
//  without "alloc" in their names, that return an integer value which is
//  supplied directly to the allocator.  Note this has the effect of
//  disabling these warnings completely anyway, because they only apply to
//  "alloc" functions and would not catch overflows as the input of the
//  bounds check.
//
//  This is quite frustrating because the class of bug these exist to catch
//  does exist in Yori and is challenging to completely eliminate.
//
#pragma warning(disable: 26831) // Allocation size might be from overflow
#pragma warning(disable: 26832) // Allocation size from narrowing conversion
#pragma warning(disable: 26833) // Potential overflow before a bounds check 
#endif
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma warning(push)
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1500)
#pragma warning(disable: 4668) // preprocessor conditional with nonexistent macro, SDK bug
#pragma warning(disable: 4255) // no function prototype given.  8.1 and earlier SDKs exhibit this.
#pragma warning(disable: 4820) // implicit padding added in structure
#endif

/**
 Indicate support for compiling for ARM32 if an SDK is available.
 */
#define _ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE 1

#include <windows.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma warning(pop)
#endif

/**
 Indicate to the standard CRT header that we're compiling the CRT itself.
 */
#define MINICRT_BUILD
#include "yoricrt.h"

#ifdef UNICODE
/**
 Indicate this compilation should generate the unicode form of tcmdlinetoargs.
 */
#define mini_tcmdlinetoargs mini_wcmdlinetoargs
#else
/**
 Indicate this compilation should generate the ansi form of tcmdlinetoargs.
 */
#define mini_tcmdlinetoargs mini_cmdlinetoargs
#endif


/**
 Parses a NULL terminated command line string into an argument count and array
 of NULL terminated strings corresponding to arguments.

 @param szCmdLine The NULL terminated command line.

 @param argc On successful completion, populated with the count of arguments.

 @return A pointer to an array of NULL terminated strings containing the
         parsed arguments.
 */
LPTSTR *
MCRT_FN
mini_tcmdlinetoargs(LPTSTR szCmdLine, int * argc)
{
    int arg_count = 0;
    int char_count = 0;
    TCHAR break_char = ' ';
    TCHAR * c;
    LPTSTR * ret;
    LPTSTR ret_str;

    //
    //  Consume all spaces.  After this, we're either at
    //  the end of string, or we have an arg, and it
    //  might start with a quote
    //

    c = szCmdLine;
    while (*c == ' ') c++;
    if (*c == '"') {
        break_char = '"';
        c++;
    }

    while (*c != '\0') {
        if (*c == break_char) {
            break_char = ' ';
            c++;
            while (*c == break_char) c++;
            if (*c == '"') {
                break_char = '"';
                c++;
            }
                arg_count++;
        } else {
            char_count++;

            //
            //  If we hit a break char, we count the argument then.
            //  If we hit end of string, count it here; note we're
            //  only counting it if we counted a character before it
            //  (ie., trailing whitespace is not an arg.)
            //

            c++;

            if (*c == '\0') {
                arg_count++;
            }
        }
    }

    *argc = arg_count;

    ret = HeapAlloc( GetProcessHeap(), 0, (arg_count * sizeof(LPTSTR)) + (char_count + arg_count) * sizeof(TCHAR));
    if (ret == NULL) {
        return ret;
    }

    ret_str = (LPTSTR)(ret + arg_count);

    arg_count = 0;
    ret[arg_count] = ret_str;

    //
    //  Consume all spaces.  After this, we're either at
    //  the end of string, or we have an arg, and it
    //  might start with a quote
    //

    c = szCmdLine;
    while (*c == ' ') c++;
    break_char = ' ';
    if (*c == '"') {
        break_char = '"';
        c++;
    }

    while (*c != '\0') {
        if (*c == break_char) {
            *ret_str = '\0';
            ret_str++;

            break_char = ' ';
            c++;
            while (*c == break_char) c++;
            if (*c == '"') {
                break_char = '"';
                c++;
            }
            if (*c != '\0') {
                arg_count++;
#if defined(_MSC_VER) && (_MSC_VER >= 1700)
                //
                //  The 2013 analyzer thinks this could overrun, but doesn't
                //  provide much detail as to why.  I think it can't follow
                //  that the logic above is the same as the logic below for
                //  calculating the argument and character count.
                //
#pragma warning(suppress: 6386)
#endif
                ret[arg_count] = ret_str;
            }
        } else {
            *ret_str = *c;
            ret_str++;

            //
            //  If we hit a break char, we count the argument then.
            //  If we hit end of string, count it here; note we're
            //  only counting it if we counted a character before it
            //  (ie., trailing whitespace is not an arg.)
            //

            c++;

            if (*c == '\0') {
                *ret_str = '\0';
            }
        }
    }

    return ret;
}


#ifdef UNICODE
/**
 Specifies the main entrypoint function for the application after arguments
 have been parsed.
 */
#define CONSOLE_USER_ENTRYPOINT wmain

/**
 Specifies the entrypoint function that Windows will invoke.
 */
#define CONSOLE_CRT_ENTRYPOINT  wmainCRTStartup
#else
/**
 Specifies the main entrypoint function for the application after arguments
 have been parsed.
 */
#define CONSOLE_USER_ENTRYPOINT main

/**
 Specifies the entrypoint function that Windows will invoke.
 */
#define CONSOLE_CRT_ENTRYPOINT  mainCRTStartup
#endif

/**
 Forward declaration of the user entrypoint.  This function should reside in
 the application being compiled, outside of the shared library code.
 */
int __cdecl CONSOLE_USER_ENTRYPOINT(int argc, LPTSTR argv[]);

/**
 The entrypoint function that the Windows loader will commence execution from.
 */
VOID __cdecl CONSOLE_CRT_ENTRYPOINT(void)
{
    TCHAR ** argv;
    int argc;
    int ret;

    argc = 0;

    argv = mini_tcmdlinetoargs(GetCommandLine(), &argc);
    if (argv == NULL) {
        ret = EXIT_FAILURE;
    } else {
        ret = CONSOLE_USER_ENTRYPOINT(argc, argv);
    }

    ExitProcess(ret);
}


// vim:sw=4:ts=4:et:
