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
#pragma warning(disable: 4514) // unreferenced inline function
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma warning(disable: 4996) // function (GetVersion) declared deprecated
#endif
#pragma warning(disable: 4702) // Unreachable code
#pragma warning(disable: 4705) // statement has no effect

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

#include <winioctl.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma warning(pop)
#else
#pragma warning(disable: 4001) // Single line comment, again
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
 The exit code of a process that indicates success.
 */
#define EXIT_SUCCESS 0

/**
 The exit code of a process that indicates failure.
 */
#define EXIT_FAILURE 1

#include <yoricrt.h>

#ifndef __in

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
 SAL annotation describing an input buffer with a specified number of
 elements.
 */
#define __in_ecount(x)

/**
 SAL annotation describing an output buffer with a specified number of
 elements.
 */
#define __out_ecount(x)

/**
 SAL annotation describing an optional output pointer whose value is changed.
 */
#define __deref_opt_out
#endif

#ifndef FILE_SHARE_DELETE
/**
 Definition for share delete for compilers that don't contain it.
 */
#define FILE_SHARE_DELETE 4
#endif

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
/**
 If the console supports it, use the VT100 processing it provides.
 */
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif


#ifndef DWORD_PTR
#ifndef _WIN64

/**
 Definition for pointer size integer for compilers that don't contain it.
 */
typedef DWORD DWORD_PTR;

/**
 Definition for pointer size integer for compilers that don't contain it.
 */
typedef ULONG ULONG_PTR;
#endif
#endif

#ifndef MAXINT_PTR
#ifndef _WIN64

/**
 Definition for native size type for compilers that don't contain it.
 */
typedef ULONG SIZE_T;
#endif
#endif

#ifndef FSCTL_SET_REPARSE_POINT

/**
 The FSCTL code to set a reparse point, if it's not already defined.
 */
#define FSCTL_SET_REPARSE_POINT CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 41, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 The FSCTL code to get a reparse point, if it's not already defined.
 */
#define FSCTL_GET_REPARSE_POINT CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 42, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#ifndef IO_REPARSE_TAG_MOUNT_POINT

/**
 The reparse tag indicating a mount point or directory junction.
 */
#define IO_REPARSE_TAG_MOUNT_POINT (0xA0000003)
#endif

#ifndef IO_REPARSE_TAG_SYMLINK
/**
 The reparse tag indicating a symbolic link.
 */
#define IO_REPARSE_TAG_SYMLINK     (0xA000000C)
#endif

#ifndef FILE_FLAG_OPEN_REPARSE_POINT
/**
 The open flag to open a reparse point rather than any link target.
 */
#define FILE_FLAG_OPEN_REPARSE_POINT 0x200000
#endif

#ifndef FILE_ATTRIBUTE_REPARSE_POINT
/**
 The file attribute indicating presence of a reparse point.
 */
#define FILE_ATTRIBUTE_REPARSE_POINT 0x400
#endif

#ifndef GUID_DEFINED

/**
 Indicate that this file just created a definition for a GUID.
 */
#define GUID_DEFINED

/**
 A structure definition for a GUID allocation.  We don't care about the
 parts in this structure.
 */
typedef struct _GUID {

    /**
     The first 32 bits of the GUID.
     */
    DWORD Part1;

    /**
     The next 16 bits of the GUID.
     */
    WORD Part2;

    /**
     The next 16 bits of the GUID.
     */
    WORD Part3;

    /**
     The next 64 bits of the GUID.
     */
    BYTE Part4[8];
} GUID;

#endif

// vim:sw=4:ts=4:et:
