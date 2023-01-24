/**
 * @file lib/condrv.c
 *
 * Yori console driver support routines
 *
 * Copyright (c) 2020-2023 Malcolm J. Smith
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
 Handle to the Condrv driver.  This is populated as needed.
 */
HANDLE YoriLibCondrvHandle;

/**
 Try to obtain the console handle from the PEB.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibGetConsoleHandleFromPeb(
    )
{
    PYORI_LIB_PEB_NATIVE Peb;
    LONG Status;
    PROCESS_BASIC_INFORMATION BasicInfo;
    DWORD dwBytesReturned;

    if (DllNtDll.pNtQueryInformationProcess == NULL) {
        return FALSE;
    }

    Status = DllNtDll.pNtQueryInformationProcess(GetCurrentProcess(), 0, &BasicInfo, sizeof(BasicInfo), &dwBytesReturned);
    if (Status != 0) {
        return FALSE;
    }

    Peb = (PYORI_LIB_PEB_NATIVE)BasicInfo.PebBaseAddress;

    if (Peb->ProcessParameters->ConsoleHandle != NULL) {
        YoriLibCondrvHandle = Peb->ProcessParameters->ConsoleHandle;
        return TRUE;
    }

    return FALSE;
}

/**
 A buffer with a length.
 */
typedef struct _CONDRV_BUFFER {

    /**
     The size of the buffer, in bytes.
     */
    DWORD Size;

#ifdef _WIN64
    /**
     This structure predates 64 bit by a lot, and it's not aligned correctly.
     Explicitly add necessary padding.
     */
    DWORD ReservedForAlignment;
#endif

    /**
     Pointer to the buffer.
     */
    PVOID Buffer;
} CONDRV_BUFFER, *PCONDRV_BUFFER;

/**
 A console API that specifies the API number and the length of its message
 in bytes.
 */
typedef struct _CONDRV_API {
    /**
     The API number.
     */
    DWORD Number;

    /**
     The size of the API message.
     */
    DWORD MsgSize;
} CONDRV_API, *PCONDRV_API;

/**
 A message description to set the display mode.
 */
typedef struct _CONDRV_SETDISPLAYMODE_MSG {
    /**
     Flags for the new display mode.
     */
    DWORD dwFlags;                 // Input

    /**
     On successful completion, populated with the new screen buffer
     dimensions.
     */
    COORD ScreenBufferDimensions;  // Output
} CONDRV_SETDISPLAYMODE_MSG, *PCONDRV_SETDISPLAYMODE_MSG;

/**
 A structure sent to Condrv for any message.
 */
typedef struct _YORI_SIMPLE_CONDRV_PACKET {

    /**
     Indicates the handle to the console.  Note this is not the same as the
     condrv handle (which is where this IOCTL is sent.)
     */
    HANDLE ConsoleClient;

    /**
     The number of input buffers.  In this simplified structure, this is
     effectively hardcoded to be 1.
     */
    DWORD InputBufferCount;

    /**
     The number of output buffers.  In this simplified structure, this is
     effectively hardcoded to be 1.
     */
    DWORD OutputBufferCount;

    /**
     The input buffer.
     */
    CONDRV_BUFFER InputBuffer;

    /**
     The output buffer.
     */
    CONDRV_BUFFER OutputBuffer;

    /**
     The API being requested.
     */
    CONDRV_API Api;

    /**
     A set of message structures for each known condrv API.
     */
    union {
        /**
         Message payload for SetConsoleDisplayMode.
         */
        CONDRV_SETDISPLAYMODE_MSG mSetDisplayMode;
    } u;
} YORI_SIMPLE_CONDRV_PACKET, *PYORI_SIMPLE_CONDRV_PACKET;

/**
 The API number for SetConsoleDisplayMode.
 */
#define CONDRV_OP_SET_DISPLAY_MODE ((3<<24) | (13))

#ifndef FILE_DEVICE_CONSOLE
/**
 The device type for a console, if not defined by the current compilation
 environment.
 */
#define FILE_DEVICE_CONSOLE (0x00000050)
#endif

/**
 The IOCTL code to issue user requests to Condrv.  The vast majority of
 requests are "user" requests, as far as it's concerned.
 */
#define IOCTL_CONDRV_ISSUE_USER_IO \
    CTL_CODE(FILE_DEVICE_CONSOLE, 5, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

/**
 A wrapper for SetConsoleDisplayMode that invokes condrv directly rather
 than using the Win32 API.  This occurs because for many releases the
 Win32 API was not compiled for 64 bit, so it needs to be invoked
 explicitly.

 @param hConsoleOutput Handle to a console output device.

 @param dwFlags The new console display mode flags.

 @param lpNewScreenBufferDimensions On successful completion, populated with
        the new screen buffer size.

 @return TRUE to indicate success, or FALSE to indicate failure.
 */
BOOL
YoriLibSetConsoleDisplayMode(
    __in HANDLE hConsoleOutput,
    __in DWORD dwFlags,
    __out PCOORD lpNewScreenBufferDimensions
    )
{
    YORI_SIMPLE_CONDRV_PACKET Packet;
    DWORD BytesReturned;
    DWORD OsMajor;
    DWORD OsMinor;
    DWORD OsBuild;

    //
    //  The current condrv architecture was in flux until Windows 8.1.  This
    //  function exists to patch behavior on Windows 10 or 8.1 with an
    //  upgraded conhost.  If the OS is older than that, fail upfront.
    //

    YoriLibGetOsVersion(&OsMajor, &OsMinor, &OsBuild);
    if (OsMajor < 6 ||
        (OsMajor == 6 && OsMinor < 3)) {

        return FALSE;
    }

    if (YoriLibCondrvHandle == NULL) {
        if (!YoriLibGetConsoleHandleFromPeb()) {
            return FALSE;
        }
    }

    Packet.ConsoleClient = hConsoleOutput;
    Packet.InputBufferCount = 1;
    Packet.OutputBufferCount = 1;
    Packet.InputBuffer.Size = sizeof(CONDRV_API) + sizeof(Packet.u.mSetDisplayMode);
    Packet.InputBuffer.Buffer = &Packet.Api;
    Packet.OutputBuffer.Size = sizeof(Packet.u.mSetDisplayMode);
    Packet.OutputBuffer.Buffer = &Packet.u.mSetDisplayMode;
    Packet.Api.Number = CONDRV_OP_SET_DISPLAY_MODE;
    Packet.Api.MsgSize = sizeof(Packet.u.mSetDisplayMode);

    Packet.u.mSetDisplayMode.dwFlags = dwFlags;

    if (!DeviceIoControl(YoriLibCondrvHandle,
                         IOCTL_CONDRV_ISSUE_USER_IO,
                         &Packet,
                         FIELD_OFFSET(YORI_SIMPLE_CONDRV_PACKET, u) + sizeof(Packet.u.mSetDisplayMode),
                         NULL,
                         0,
                         &BytesReturned,
                         NULL)) {

        return FALSE;
    }

    *lpNewScreenBufferDimensions = Packet.u.mSetDisplayMode.ScreenBufferDimensions;

    return TRUE;
}

// vim:sw=4:ts=4:et:
