/**
 * @file shutdn/shutdn.c
 *
 * Yori shell shutdown the system.
 *
 * Copyright (c) 2019 Malcolm J. Smith
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
CHAR strShutdownHelpText[] =
        "\n"
        "Shutdown the system.\n"
        "\n"
        "SHUTDN [-license] [-f] [-k|-l|-r|-s [-p]]\n"
        "\n"
        "   -f             Force the action without waiting for programs to close cleanly\n"
        "   -k             Lock the current session\n"
        "   -l             Log off the current user\n"
        "   -p             Turn off power after shutdown, if supported\n"
        "   -r             Reboot the system\n"
        "   -s             Shutdown the system\n";

/**
 Display usage text to the user.
 */
BOOL
ShutdownHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Shutdown %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strShutdownHelpText);
    return TRUE;
}

/**
 If the privilege for shutting down the system is available to the process,
 enable it.

 @return TRUE to indicate the privilege was enabled, FALSE if it was not.
 */
BOOL
ShutdownEnableShutdownPrivilege(VOID)
{
    TOKEN_PRIVILEGES TokenPrivileges;
    LUID ShutdownLuid;
    HANDLE TokenHandle;

    YoriLibLoadAdvApi32Functions();
    if (DllAdvApi32.pLookupPrivilegeValueW == NULL ||
        DllAdvApi32.pOpenProcessToken == NULL ||
        DllAdvApi32.pAdjustTokenPrivileges == NULL) {

        return FALSE;
    }

    if (!DllAdvApi32.pLookupPrivilegeValueW(NULL, SE_SHUTDOWN_NAME, &ShutdownLuid)) {
        return FALSE;
    }

    if (!DllAdvApi32.pOpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &TokenHandle)) {
        return FALSE;
    }

    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0].Luid = ShutdownLuid;
    TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!DllAdvApi32.pAdjustTokenPrivileges(TokenHandle, FALSE, &TokenPrivileges, 0, NULL, 0)) {
        CloseHandle(TokenHandle);
        return FALSE;
    }
    CloseHandle(TokenHandle);

    return TRUE;
}

/**
 The set of operations supported by this command.
 */
typedef enum _SHUTDN_OP {
    ShutdownOpUsage = 0,
    ShutdownOpLogoff = 1,
    ShutdownOpShutdown = 2,
    ShutdownOpReboot = 3,
    ShutdownOpLock = 4
} SHUTDN_OP;

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the shutdown builtin command.
 */
#define ENTRYPOINT YoriCmd_YSHUTDN
#else
/**
 The main entrypoint for the shutdown standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the shutdown cmdlet.

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
    BOOL Result;
    DWORD i;
    DWORD StartArg = 0;
    YORI_STRING Arg;
    SHUTDN_OP Op;
    LPTSTR ErrText;
    BOOL Force;
    BOOL Powerdown;
    DWORD ShutdownOptions;
    DWORD Err;

    Op = ShutdownOpUsage;
    Force = FALSE;
    Powerdown = FALSE;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                ShutdownHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("f")) == 0) {
                Force = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("k")) == 0) {
                Op = ShutdownOpLock;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("l")) == 0) {
                Op = ShutdownOpLogoff;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("p")) == 0) {
                Powerdown = TRUE;
                Op = ShutdownOpShutdown;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("r")) == 0) {
                Op = ShutdownOpReboot;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                Op = ShutdownOpShutdown;
                ArgumentUnderstood = TRUE;
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

    YoriLibLoadAdvApi32Functions();
    YoriLibLoadUser32Functions();

    ShutdownEnableShutdownPrivilege();

    Result = TRUE;

    switch(Op) {
        case ShutdownOpUsage:
            ShutdownHelp();
            break;
        case ShutdownOpLogoff:
            if (DllUser32.pExitWindowsEx == NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("shutdn: OS support not present\n"));
                return EXIT_FAILURE;
            }
            ShutdownOptions = EWX_LOGOFF;
            if (Force) {
                ShutdownOptions = ShutdownOptions | EWX_FORCE;
            }
            Result = DllUser32.pExitWindowsEx(ShutdownOptions, 0);
            if (!Result) {
                ErrText = YoriLibGetWinErrorText(GetLastError());
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Logoff failed: %s"), ErrText);
                YoriLibFreeWinErrorText(ErrText);
            }
            break;
        case ShutdownOpReboot:
            if (DllUser32.pExitWindowsEx == NULL && DllAdvApi32.pInitiateShutdownW == NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("shutdn: OS support not present\n"));
                return EXIT_FAILURE;
            }
            Err = ERROR_SUCCESS;
            if (DllUser32.pExitWindowsEx != NULL) {
                ShutdownOptions = EWX_REBOOT;
                if (Force) {
                    ShutdownOptions = ShutdownOptions | EWX_FORCE;
                }
                Result = DllUser32.pExitWindowsEx(ShutdownOptions, 0);
                if (!Result) {
                    Err = GetLastError();
                }
            } else {
                ShutdownOptions = SHUTDOWN_RESTART;
                if (Force) {
                    ShutdownOptions = ShutdownOptions | SHUTDOWN_FORCE_OTHERS | SHUTDOWN_FORCE_SELF;
                }
                Err = DllAdvApi32.pInitiateShutdownW(NULL, NULL, 0, ShutdownOptions, 0);
            }
            if (Err != ERROR_SUCCESS) {
                ErrText = YoriLibGetWinErrorText(Err);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Reboot failed: %s"), ErrText);
                YoriLibFreeWinErrorText(ErrText);
            }
            break;
        case ShutdownOpShutdown:
            if (DllUser32.pExitWindowsEx == NULL && DllAdvApi32.pInitiateShutdownW == NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("shutdn: OS support not present\n"));
                return EXIT_FAILURE;
            }
            Err = ERROR_SUCCESS;
            if (DllUser32.pExitWindowsEx != NULL) {
                ShutdownOptions = EWX_SHUTDOWN;
                if (Force) {
                    ShutdownOptions = ShutdownOptions | EWX_FORCE;
                }
                if (Powerdown) {
                    ShutdownOptions = ShutdownOptions | EWX_POWEROFF;
                }
                if (!DllUser32.pExitWindowsEx(ShutdownOptions, 0)) {
                    Err = GetLastError();
                }
            } else {
                ShutdownOptions = SHUTDOWN_NOREBOOT;
                if (Powerdown) {
                    ShutdownOptions = SHUTDOWN_POWEROFF;
                }

                if (Force) {
                    ShutdownOptions = ShutdownOptions | SHUTDOWN_FORCE_OTHERS | SHUTDOWN_FORCE_SELF;
                }
                Err = DllAdvApi32.pInitiateShutdownW(NULL, NULL, 0, ShutdownOptions, 0);
            }
            if (!Result) {
                ErrText = YoriLibGetWinErrorText(Err);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Shutdown failed: %s"), ErrText);
                YoriLibFreeWinErrorText(ErrText);
            }
            break;
        case ShutdownOpLock:
            if (DllUser32.pLockWorkStation == NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("shutdn: OS support not present\n"));
                return EXIT_FAILURE;
            }
            DllUser32.pLockWorkStation();
            break;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
