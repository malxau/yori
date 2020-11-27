/**
 * @file lib/group.c
 *
 * Yori group membership routines
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

#include "yoripch.h"
#include "yorilib.h"


/**
 Query whether the current process is running as part of the specified group.

 @param GroupName The group name to check whether the process is running with
        the group in its token.

 @param IsMember On successful completion, set to TRUE to indicate the process
        is running in the context of the specified group, FALSE if not.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibIsCurrentUserInGroup(
    __in PYORI_STRING GroupName,
    __out PBOOL IsMember
    )
{
    union {
        SID Sid;
        UCHAR Storage[512];
    } Sid;
    TCHAR Domain[256];
    DWORD SidSize;
    DWORD DomainNameSize;
    SID_NAME_USE Use;

    ASSERT(YoriLibIsStringNullTerminated(GroupName));

    YoriLibLoadAdvApi32Functions();

    if (DllAdvApi32.pLookupAccountNameW == NULL ||
        DllAdvApi32.pCheckTokenMembership == NULL) {

        return FALSE;
    }

    SidSize = sizeof(Sid);
    DomainNameSize = sizeof(Domain);

    if (!DllAdvApi32.pLookupAccountNameW(NULL, GroupName->StartOfString, &Sid, &SidSize, Domain, &DomainNameSize, &Use)) {
        return FALSE;
    }

    if (Use != SidTypeGroup && Use != SidTypeWellKnownGroup && Use != SidTypeAlias) {
        return FALSE;
    }

    if (!DllAdvApi32.pCheckTokenMembership(NULL, &Sid.Sid, IsMember)) {
        return FALSE;
    }

    return TRUE;
}

/**
 Query whether the current process is running as part of the specified group.

 @param GroupId The well known group identifier to check whether the process
        is running with the group in its token.

 @param IsMember On successful completion, set to TRUE to indicate the process
        is running in the context of the specified group, FALSE if not.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibIsCurrentUserInWellKnownGroup(
    __in DWORD GroupId,
    __out PBOOL IsMember
    )
{
    PSID Sid;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    YoriLibLoadAdvApi32Functions();

    if (DllAdvApi32.pAllocateAndInitializeSid == NULL ||
        DllAdvApi32.pCheckTokenMembership == NULL ||
        DllAdvApi32.pFreeSid == NULL) {

        return FALSE;
    }

    if (!DllAdvApi32.pAllocateAndInitializeSid(&NtAuthority,
                                               2,
                                               SECURITY_BUILTIN_DOMAIN_RID,
                                               GroupId,
                                               0,
                                               0,
                                               0,
                                               0,
                                               0,
                                               0,
                                               &Sid)) {

        return FALSE;
    }

    if (!DllAdvApi32.pCheckTokenMembership(NULL, Sid, IsMember)) {
        DllAdvApi32.pFreeSid(Sid);
        return FALSE;
    }

    DllAdvApi32.pFreeSid(Sid);
    return TRUE;
}

// vim:sw=4:ts=4:et:
