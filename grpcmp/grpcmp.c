/**
 * @file grpcmp/grpcmp.c
 *
 * Yori shell query group membership
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
CHAR strHelpText[] =
        "\n"
        "Returns true if the user is a member of the specified group."
        "\n"
        "GRPCMP <group>\n";

/**
 Display usage text to the user.
 */
BOOL
GrpcmpHelp()
{
    YORI_STRING License;

    YoriLibMitLicenseText(_T("2018"), &License);

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Grpcmp %i.%i\n"), GRPCMP_VER_MAJOR, GRPCMP_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs\n"), strHelpText);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &License);
    YoriLibFreeStringContents(&License);
    return TRUE;
}

/**
 Prototype for the CheckTokenMembership function.
 */
typedef BOOL WINAPI CHECK_TOKEN_MEMBERSHIP_FN(HANDLE, PSID, PBOOL);

/**
 Prototype for a pointer to the CheckTokenMembership function.
 */
typedef CHECK_TOKEN_MEMBERSHIP_FN *PCHECK_TOKEN_MEMBERSHIP_FN;

/**
 The main entrypoint for the grpcmp cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ymain(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    union {
        SID Sid;
        UCHAR Storage[512];
    } Sid;
    TCHAR Domain[256];
    DWORD SidSize;
    DWORD DomainNameSize;
    DWORD StartArg;
    DWORD i;
    SID_NAME_USE Use;
    BOOL IsMember = FALSE;
    HANDLE hAdvApi = GetModuleHandle(_T("ADVAPI32"));
    PCHECK_TOKEN_MEMBERSHIP_FN CheckTokenMembershipFn;
    YORI_STRING Arg;

    StartArg = 0;

    for (i = 1; i < ArgC; i++) {

        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));
        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                GrpcmpHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("-")) == 0) {
                StartArg = i;
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

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("grpcmp: missing argument\n"));
        return EXIT_FAILURE;
    }

    SidSize = sizeof(Sid);
    DomainNameSize = sizeof(Domain);

    if (!LookupAccountName(NULL, ArgV[StartArg].StartOfString, &Sid, &SidSize, Domain, &DomainNameSize, &Use)) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("grpcmp: could not find group: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return EXIT_FAILURE;
    }

    if (Use != SidTypeGroup && Use != SidTypeWellKnownGroup && Use != SidTypeAlias) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("grpcmp: object is not a group, type %i\n"), Use);
        return EXIT_FAILURE;
    }

    CheckTokenMembershipFn = (PCHECK_TOKEN_MEMBERSHIP_FN)GetProcAddress(hAdvApi, "CheckTokenMembership");
    if (CheckTokenMembershipFn == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("grpcmp: OS functionality not available\n"));
        return EXIT_FAILURE;
    }

    if (CheckTokenMembershipFn(NULL, &Sid.Sid, &IsMember)) {
        if (IsMember) {
            return EXIT_SUCCESS;
        }
    }

    return EXIT_FAILURE;
}

// vim:sw=4:ts=4:et:
