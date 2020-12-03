/**
 * @file lib/movefile.c
 *
 * Yori shell move or rename a file
 *
 * Copyright (c) 2017-2020 Malcolm J. Smith
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, MOVEESS OR
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
 Rename or move a file or directory.  This routine will copy and replace
 files as needed to complete the request.

 @param Source Pointer to the source of the rename.

 @param FullDest Pointer to the target of the rename.

 @param ReplaceExisting If TRUE, any existing file is overwritten.  If FALSE,
        existing files are retained and the call fails with last error set
        appropriately.

 @return TRUE to indicate success, FALSE to indicate failure.  On failure,
         last error is populated with the error condition.
 */
BOOLEAN
YoriLibMoveFile(
    __in PYORI_STRING Source,
    __in PYORI_STRING FullDest,
    __in BOOLEAN ReplaceExisting
    )
{
    DWORD OsMajor;
    DWORD OsMinor;
    DWORD OsBuild;
    DWORD Flags;

    ASSERT(YoriLibIsStringNullTerminated(Source));
    ASSERT(YoriLibIsStringNullTerminated(FullDest));

    Flags = MOVEFILE_COPY_ALLOWED;
    if (ReplaceExisting) {
        Flags = Flags | MOVEFILE_REPLACE_EXISTING;
    }

    if (!MoveFileEx(Source->StartOfString, FullDest->StartOfString, ReplaceExisting)) {
        return FALSE;
    }

    YoriLibGetOsVersion(&OsMajor, &OsMinor, &OsBuild);

    //
    //  Windows 2000 and above claim to support inherited ACLs, except they
    //  really depend on applications to perform the inheritance.  On these
    //  systems, attempt to reset the ACL on the target, which really resets
    //  security and picks up state from the parent.  Don't do this on older
    //  systems, because that will result in a file with no ACL.  Note this
    //  can fail due to not having access on the file, or a file system
    //  that doesn't support security, or because the file is no longer there,
    //  so errors here are just ignored.
    //

    if (OsMajor >= 5) {
        ACL EmptyAcl;

        //
        //  This function should exist on NT4+, and callers of this function
        //  should have attempted to load AdvApi32
        //

        ASSERT(DllAdvApi32.pSetNamedSecurityInfoW != NULL &&
               DllAdvApi32.pInitializeAcl != NULL);

        if (DllAdvApi32.pInitializeAcl != NULL &&
            DllAdvApi32.pSetNamedSecurityInfoW != NULL) {

            memset(&EmptyAcl, 0, sizeof(EmptyAcl));

            DllAdvApi32.pInitializeAcl(&EmptyAcl, sizeof(EmptyAcl), ACL_REVISION);
            DllAdvApi32.pSetNamedSecurityInfoW(FullDest->StartOfString, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION, NULL, NULL, &EmptyAcl, NULL);
        }
    }

    return TRUE;
}

// vim:sw=4:ts=4:et:
