/**
 * @file lib/progman.c
 *
 * Yori Program Manager DDE interfaces
 *
 * Copyright (c) 2021 Malcolm J. Smith
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
 Pointless callback because DDEML expects it.

 @param Type Ignored.

 @param Fmt Ignored.

 @param ConvHandle Ignored.

 @param StringHandle1 Ignored.

 @param StringHandle2 Ignored.

 @param DataHandle Ignored.

 @param Data1 Ignored.

 @param Data2 Ignored.

 @return NULL.
 */
HDDEDATA CALLBACK
YoriLibDdeCallback(
    __in DWORD Type,
    __in DWORD Fmt,
    __in HCONV ConvHandle,
    __in HSZ StringHandle1,
    __in HSZ StringHandle2,
    __in HDDEDATA DataHandle,
    __in DWORD_PTR Data1,
    __in DWORD_PTR Data2
    )
{
    UNREFERENCED_PARAMETER(Type);
    UNREFERENCED_PARAMETER(Fmt);
    UNREFERENCED_PARAMETER(ConvHandle);
    UNREFERENCED_PARAMETER(StringHandle1);
    UNREFERENCED_PARAMETER(StringHandle2);
    UNREFERENCED_PARAMETER(DataHandle);
    UNREFERENCED_PARAMETER(Data1);
    UNREFERENCED_PARAMETER(Data2);
    return (HDDEDATA)NULL;
}

/**
 Check if the string contains characters which would need to be escaped to
 communicate over DDE.

 @param String Pointer to the string to check.

 @return TRUE to indicate the string contains invalid chars, FALSE if it
         does not.
 */
BOOL
YoriLibDoesStringContainDDEInvalidChars(
    __in PCYORI_STRING String
    )
{
    DWORD Index;

    //
    //  A string better not contain an argument seperator or command
    //  terminator character.
    //

    for (Index = 0; Index < String->LengthInChars; Index++) {
        if (String->StartOfString[Index] == ',' ||
            String->StartOfString[Index] == ')') {

            return TRUE;
        }
    }

    return FALSE;
}

/**
 Create or modify a Program Manager item.

 @param GroupName Pointer to the name of the Program Manager group.

 @param ItemName The name of the Program Manager item.

 @param ItemPath The path that the Program Manager item should point to.

 @param WorkingDirectory If specified, the current directory to set when
        launching the executable.

 @param IconPath If specified, the path to the binary containing the icon for
        the shortcut.

 @param IconIndex The index of the icon within any executable or DLL used as
        the source of the icon.  This is ignored unless IconPath is specified.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibAddProgmanItem(
    __in PCYORI_STRING GroupName,
    __in PCYORI_STRING ItemName,
    __in PCYORI_STRING ItemPath,
    __in_opt PCYORI_STRING WorkingDirectory,
    __in_opt PCYORI_STRING IconPath,
    __in DWORD IconIndex
    )
{
    HSZ ProgmanHandle;
    HDDEDATA CommandHandle;
    HCONV ConvHandle;
    DWORD DdeInstance;
    DWORD Result;
    CONVCONTEXT ConvContext;
    YORI_STRING CommandString;

    //
    //  Check for characters that can't be communicated over DDE.  There's an
    //  escaping protocol for these, but it hardly seems worth implementing
    //  given that this function is very unlikely to see these characters.
    //

    if (YoriLibDoesStringContainDDEInvalidChars(GroupName) ||
        YoriLibDoesStringContainDDEInvalidChars(ItemName) ||
        YoriLibDoesStringContainDDEInvalidChars(ItemPath)) {

        return FALSE;
    }

    if (WorkingDirectory != NULL && YoriLibDoesStringContainDDEInvalidChars(WorkingDirectory)) {
        return FALSE;
    }

    if (IconPath != NULL && YoriLibDoesStringContainDDEInvalidChars(IconPath)) {
        return FALSE;
    }

    YoriLibLoadUser32Functions();

    if (DllUser32.pDdeClientTransaction == NULL ||
        DllUser32.pDdeConnect == NULL ||
        DllUser32.pDdeCreateDataHandle == NULL ||
        DllUser32.pDdeCreateStringHandleW == NULL ||
        DllUser32.pDdeDisconnect == NULL ||
        DllUser32.pDdeFreeStringHandle == NULL ||
        DllUser32.pDdeInitializeW == NULL ||
        DllUser32.pDdeUninitialize == NULL) {

        return FALSE;
    }

    //
    //  Initialize DDE and connect to Program Manager.
    //

    DdeInstance = 0;
    if (DllUser32.pDdeInitializeW(&DdeInstance, (PFNCALLBACK)YoriLibDdeCallback, APPCMD_CLIENTONLY, 0) != DMLERR_NO_ERROR) {
        return FALSE;
    }

    ProgmanHandle = DllUser32.pDdeCreateStringHandleW(DdeInstance, _T("PROGMAN"), CP_WINUNICODE);
    if (ProgmanHandle == (HSZ)NULL) {
        DllUser32.pDdeUninitialize(DdeInstance);
        return FALSE;
    }

    ZeroMemory(&ConvContext, sizeof(ConvContext));
    ConvContext.cb = sizeof(ConvContext);
    ConvHandle = DllUser32.pDdeConnect(DdeInstance, ProgmanHandle, ProgmanHandle, &ConvContext);
    DllUser32.pDdeFreeStringHandle(DdeInstance, ProgmanHandle);
    if (ConvHandle == (HCONV)NULL) {
        DllUser32.pDdeUninitialize(DdeInstance);
        return FALSE;
    }

    //
    //  Create or switch to the group.
    //

    YoriLibInitEmptyString(&CommandString);
    YoriLibYPrintf(&CommandString, _T("[CreateGroup(%y)]"), GroupName);
    if (CommandString.StartOfString == NULL) {
        DllUser32.pDdeDisconnect(ConvHandle);
        DllUser32.pDdeUninitialize(DdeInstance);
        return FALSE;
    }

    CommandHandle = DllUser32.pDdeCreateDataHandle(DdeInstance, (LPBYTE)CommandString.StartOfString, (CommandString.LengthInChars + 1) * sizeof(TCHAR), 0, (HSZ)NULL, CF_UNICODETEXT, 0);
    if (CommandHandle == (HDDEDATA)NULL) {
        YoriLibFreeStringContents(&CommandString);
        DllUser32.pDdeDisconnect(ConvHandle);
        DllUser32.pDdeUninitialize(DdeInstance);
        return FALSE;
    }

    if (DllUser32.pDdeClientTransaction((LPBYTE)CommandHandle, 0xFFFFFFFF, ConvHandle, (HSZ)NULL, 0, XTYP_EXECUTE, 3000, &Result) == (HDDEDATA)NULL) {
        //
        //  Note that CommandHandle may be leaked here.  Avoiding this
        //  requires establishing what happened (whether it made it to
        //  the remote side or not.)
        //
        YoriLibFreeStringContents(&CommandString);
        DllUser32.pDdeDisconnect(ConvHandle);
        DllUser32.pDdeUninitialize(DdeInstance);
        return FALSE;
    }

    //
    //  At this point CommandHandle is owned by the remote side.
    //
    //  Create the program item.
    //

    CommandString.LengthInChars = 0;
    if (IconPath != NULL && WorkingDirectory != NULL) {
        YoriLibYPrintf(&CommandString, _T("[AddItem(\"%y\",%y,\"%y\",%i,-1,-1,\"%y\")]"), ItemPath, ItemName, IconPath, IconIndex, WorkingDirectory);
    } else if (WorkingDirectory != NULL) {
        YoriLibYPrintf(&CommandString, _T("[AddItem(\"%y\",%y,,,-1,-1,\"%y\")]"), ItemPath, ItemName, WorkingDirectory);
    } else if (IconPath != NULL) {
        YoriLibYPrintf(&CommandString, _T("[AddItem(\"%y\",%y,\"%y\",%i)]"), ItemPath, ItemName, IconPath, IconIndex);
    } else {
        YoriLibYPrintf(&CommandString, _T("[AddItem(\"%y\",%y)]"), ItemPath, ItemName);
    }

    if (CommandString.LengthInChars == 0) {
        YoriLibFreeStringContents(&CommandString);
        DllUser32.pDdeDisconnect(ConvHandle);
        DllUser32.pDdeUninitialize(DdeInstance);
        return FALSE;
    }

    CommandHandle = DllUser32.pDdeCreateDataHandle(DdeInstance, (LPBYTE)CommandString.StartOfString, (CommandString.LengthInChars + 1) * sizeof(TCHAR), 0, (HSZ)NULL, CF_UNICODETEXT, 0);
    if (CommandHandle == (HDDEDATA)NULL) {
        YoriLibFreeStringContents(&CommandString);
        DllUser32.pDdeDisconnect(ConvHandle);
        DllUser32.pDdeUninitialize(DdeInstance);
        return FALSE;
    }

    if (DllUser32.pDdeClientTransaction((LPBYTE)CommandHandle, 0xFFFFFFFF, ConvHandle, (HSZ)NULL, 0, XTYP_EXECUTE, 3000, &Result) == (HDDEDATA)NULL) {
        //
        //  Note that CommandHandle may be leaked here.  Avoiding this
        //  requires establishing what happened (whether it made it to
        //  the remote side or not.)
        //
        YoriLibFreeStringContents(&CommandString);
        DllUser32.pDdeDisconnect(ConvHandle);
        DllUser32.pDdeUninitialize(DdeInstance);
        return FALSE;
    }

    YoriLibFreeStringContents(&CommandString);
    DllUser32.pDdeDisconnect(ConvHandle);
    DllUser32.pDdeUninitialize(DdeInstance);

    return TRUE;
}


// vim:sw=4:ts=4:et:
