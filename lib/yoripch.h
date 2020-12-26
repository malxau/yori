/**
 * @file lib/yoripch.h
 *
 * Yori shell master header file
 *
 * Copyright (c) 2017-2018 Malcolm J. Smith
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

/**
 Standard define to include somewhat less of the Windows headers.  Command
 line software doesn't need much.
 */
#define WIN32_LEAN_AND_MEAN 1

#pragma warning(disable: 4001) /* Single line comment, first for obvious reasons */
#pragma warning(disable: 4057) // differs in direction to slightly different base type
#pragma warning(disable: 4068) // unknown pragma
#pragma warning(disable: 4127) // conditional expression constant
#pragma warning(disable: 4201) // nameless struct/union
#pragma warning(disable: 4214) // bit field type other than int
#pragma warning(disable: 4276) // No prototype because the compiler discarded it
//#pragma warning(disable: 4514) // unreferenced inline function
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma warning(disable: 4996) // function (GetVersion) declared deprecated
#endif
#pragma warning(disable: 4702) // Unreachable code

#if defined(_MSC_VER) && (_MSC_VER <= 1500)
#pragma warning(disable: 4705) // statement has no effect
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(disable: 4668) // preprocessor conditional with nonexistent macro, SDK bug
#pragma warning(disable: 4820) // implicit padding added in structure
#pragma warning(disable: 5045) // spectre mitigation 
#pragma warning(disable: 4061) // not all enumerators handled in switch statement
#pragma warning(disable: 4062) // not all enumerators handled in switch statement
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma warning(push)
#endif

#include <windows.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma warning(pop)
#pragma warning(push)
#else
#pragma warning(disable: 4001) // Single line comment, again
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1700)
//
//  This is a serious warning.  Note this is under a warning push and there's
//  a pop after including winioctl.h.  It's just here to handle a buggy
//  windows header.
//
#pragma warning(disable: 6001) // Using uninitialized memory
#endif

#include <winioctl.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma warning(pop)
#else
#pragma warning(disable: 4001) // Single line comment, again
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(disable: 26451) // Arithmetic overflow
#endif

#ifdef _UNICODE
/**
 When Unicode is defined, indicate the literal string is Unicode.
 */
#define _T(x) L##x
#else
/**
 When Unicode is not defined, indicate the literal string is not Unicode.
 */
#define _T(x) x
#endif

/**
 The maximum stream name length in characters.  This is from
 WIN32_FIND_STREAM_DATA.
 */
#define YORI_LIB_MAX_STREAM_NAME          (MAX_PATH + 36)

/**
 The maximum file name size in characters, exclusive of path.  This is from
 WIN32_FIND_DATA.
 */
#define YORI_LIB_MAX_FILE_NAME            (MAX_PATH)

#include <yoricrt.h>

#ifndef __in

/**
 SAL annotation indicating that a condition must be true.
 */
#define __analysis_assume(x)

/**
 SAL annotation for an input value.
 */
#define __in

/**
 SAL annotation for an optional input value.
 */
#define __in_opt

/**
 SAL annotation for an output value.
 */
#define __out

/**
 SAL annotation for an optional output value.
 */
#define __out_opt

/**
 SAL annotation for a value populated on input and updated on output.
 */
#define __inout

/**
 SAL annotation for an optional value populated on input and updated on output.
 */
#define __inout_opt

/**
 SAL annotation describing an input buffer with a specified number of
 elements.
 */
#define __in_ecount(x)

/**
 SAL annotation describing an optional input buffer with a specified number of
 elements.
 */
#define __in_ecount_opt(x)

/**
 SAL annotation describing an output buffer with a specified number of
 elements.
 */
#define __out_ecount(x)

/**
 SAL annotation describing an optional output pointer whose value is changed.
 */
#define __deref_opt_out

/**
 SAL annotation describing a required output pointer whose value may be
 changed.
 */
#define __deref_out_opt

/**
 SAL annotation describing an optional output pointer whose value may be
 changed.
 */
#define __deref_opt_out_opt

/**
 SAL annotation for a return type.
 */
#define __success(x)

#endif

#ifndef _Return_type_success_
/**
 SAL annotation for a return type.
 */
#define _Return_type_success_(x)
#endif

#ifndef _Post_satisfies_
/**
 SAL annotation for a postcondition.
 */
#define _Post_satisfies_(x)
#endif

#ifndef _Acquires_lock_
/**
 SAL annotation to indicate a function acquires a lock.
 */
#define _Acquires_lock_(x)
#endif


#include <yoricmpt.h>

// vim:sw=4:ts=4:et:
