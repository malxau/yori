/**
 * @file shutdn/shutdn.c
 *
 * Yori shell shutdown the system.
 *
 * Copyright (c) 2019-2024 Malcolm J. Smith
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
        "SHUTDN [-license] [-f] [-d|-e|-h|-k|-l|-r|-s [-p]]\n"
        "\n"
        "   -d             Disconnect the current session\n"
        "   -e             Sleep the system\n"
        "   -f             Force the action without waiting for programs to close cleanly\n"
        "   -h             Hibernates the system\n"
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
 The set of operations supported by this command.
 */
typedef enum _SHUTDN_OP {
    ShutdownOpUsage = 0,
    ShutdownOpLogoff = 1,
    ShutdownOpShutdown = 2,
    ShutdownOpReboot = 3,
    ShutdownOpLock = 4,
    ShutdownOpDisconnect = 5,
    ShutdownOpSleep = 6,
    ShutdownOpHibernate = 7,
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
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOLEAN ArgumentUnderstood;
    BOOL Result;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 0;
    YORI_STRING Arg;
    SHUTDN_OP Op;
    LPTSTR ErrText;
    BOOLEAN Force;
    BOOLEAN Powerdown;
    DWORD ShutdownOptions;
    DWORD Err;

    Op = ShutdownOpUsage;
    Force = FALSE;
    Powerdown = FALSE;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                ShutdownHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2019-2024"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("d")) == 0) {
                Op = ShutdownOpDisconnect;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("e")) == 0) {
                Op = ShutdownOpSleep;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("f")) == 0) {
                Force = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("h")) == 0) {
                Op = ShutdownOpHibernate;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("k")) == 0) {
                Op = ShutdownOpLock;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("l")) == 0) {
                Op = ShutdownOpLogoff;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("p")) == 0) {
                Powerdown = TRUE;
                Op = ShutdownOpShutdown;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("r")) == 0) {
                Op = ShutdownOpReboot;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("s")) == 0) {
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
    YoriLibLoadPowrprofFunctions();
    YoriLibLoadUser32Functions();

    YoriLibEnableShutdownPrivilege();

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
                return EXIT_FAILURE;
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
                return EXIT_FAILURE;
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
                Result = DllUser32.pExitWindowsEx(ShutdownOptions, 0);
                if (!Result) {
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
                return EXIT_FAILURE;
            }
            break;
        case ShutdownOpLock:
            if (DllUser32.pLockWorkStation == NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("shutdn: OS support not present\n"));
                return EXIT_FAILURE;
            }
            DllUser32.pLockWorkStation();
            break;
        case ShutdownOpDisconnect:
            YoriLibLoadWtsApi32Functions();
            if (DllWtsApi32.pWTSDisconnectSession == NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("shutdn: OS support not present\n"));
                return EXIT_FAILURE;
            }

            Result = DllWtsApi32.pWTSDisconnectSession(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, FALSE);
            if (!Result) {
                Err = GetLastError();
                ErrText = YoriLibGetWinErrorText(Err);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Disconnect failed: %s"), ErrText);
                YoriLibFreeWinErrorText(ErrText);
                return EXIT_FAILURE;
            }
            break;
        case ShutdownOpSleep:
            if (DllKernel32.pSetSystemPowerState == NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("shutdn: OS support not present\n"));
                return EXIT_FAILURE;
            }
            if (DllPowrprof.pIsPwrSuspendAllowed != NULL &&
                !DllPowrprof.pIsPwrSuspendAllowed()) {

                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("shutdn: system does not support sleep\n"));
                return EXIT_FAILURE;
            }
            Result = DllKernel32.pSetSystemPowerState(TRUE, Force);
            if (!Result) {
                Err = GetLastError();
                ErrText = YoriLibGetWinErrorText(Err);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Sleep failed: %s"), ErrText);
                YoriLibFreeWinErrorText(ErrText);
                return EXIT_FAILURE;
            }
            break;
        case ShutdownOpHibernate:
            if (DllKernel32.pSetSystemPowerState == NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("shutdn: OS support not present\n"));
                return EXIT_FAILURE;
            }
            if (DllPowrprof.pIsPwrHibernateAllowed != NULL &&
                !DllPowrprof.pIsPwrHibernateAllowed()) {

                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("shutdn: system does not support hibernate\n"));
                return EXIT_FAILURE;
            }
            Result = DllKernel32.pSetSystemPowerState(FALSE, Force);
            if (!Result) {
                Err = GetLastError();
                ErrText = YoriLibGetWinErrorText(Err);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Hibernate failed: %s"), ErrText);
                YoriLibFreeWinErrorText(ErrText);
                return EXIT_FAILURE;
            }
            break;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
