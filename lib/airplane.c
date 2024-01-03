/**
 * @file lib/airplane.c
 *
 * Helper routines for manipulating airplane mode
 *
 * Copyright (c) 2023 Malcolm Smith
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

#pragma warning(disable: 4226)

/**
 A declaration for a GUID defining the radio API interface.
 */
const GUID CLSID_RadioManagement = { 0x581333F6L, 0x28DB, 0x41BE, { 0xBC, 0x7A, 0xFF, 0x20, 0x1F, 0x12, 0xF3, 0xF6 } };

/**
 The IRadioManager interface.
 */
const GUID IID_IRadioManager = { 0xDB3AFBFBL, 0x08E6, 0x46C6, { 0xAA, 0x70, 0xBF, 0x9A, 0x34, 0xC3, 0x0A, 0xB7 } };

/**
 Query Airplane Mode state.

 @param AirplaneModeEnabled On successful completion, set to TRUE to indicate
        airplane mode is enabled; set to FALSE to indicate it is disabled.

 @param AirplaneModeChangable On successful completion, set to TRUE to indicate
        airplane mode can be changed; set to FALSE to indicate it cannot
        change.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriLibGetAirplaneMode(
    __out PBOOLEAN AirplaneModeEnabled,
    __out PBOOLEAN AirplaneModeChangable
    )
{
    IRadioManager *RadioManager = NULL;
    HRESULT hRes;
    BOOL bRadioEnabled;
    BOOL bUiEnabled;
    DWORD dwState;
    BOOLEAN Result;

    Result = FALSE;

    YoriLibLoadOle32Functions();
    if (DllOle32.pCoCreateInstance == NULL || DllOle32.pCoInitialize == NULL) {
        return FALSE;
    }

    hRes = DllOle32.pCoInitialize(NULL);
    if (!SUCCEEDED(hRes)) {
        return FALSE;
    }

    hRes = DllOle32.pCoCreateInstance(&CLSID_RadioManagement, NULL, CLSCTX_LOCAL_SERVER, &IID_IRadioManager, (void **)&RadioManager);
    if (!SUCCEEDED(hRes)) {
        return FALSE;
    }

    bRadioEnabled = TRUE;
    bUiEnabled = TRUE;
    dwState = 0;

    hRes = RadioManager->Vtbl->GetSystemRadioState(RadioManager, &bRadioEnabled, &bUiEnabled, &dwState);
    if (!SUCCEEDED(hRes)) {
        Result = FALSE;
        goto Exit;
    }

    if (bRadioEnabled == FALSE) {
        *AirplaneModeEnabled = TRUE;
    } else {
        *AirplaneModeEnabled = FALSE;
    }

    if (bUiEnabled) {
        *AirplaneModeChangable = TRUE;
    } else {
        *AirplaneModeChangable = FALSE;
    }

    Result = TRUE;

Exit:

    if (RadioManager != NULL) {
        RadioManager->Vtbl->Release(RadioManager);
    }

    return Result;
}

/**
 Set Airplane Mode state.

 @param AirplaneModeEnabled TRUE to indicate airplane mode should be enabled;
        FALSE to indicate it should be disabled.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriLibSetAirplaneMode(
    __in BOOLEAN AirplaneModeEnabled
    )
{
    IRadioManager *RadioManager = NULL;
    HRESULT hRes;
    BOOL bRadioEnabled;
    BOOLEAN Result;

    Result = FALSE;

    YoriLibLoadOle32Functions();
    if (DllOle32.pCoCreateInstance == NULL || DllOle32.pCoInitialize == NULL) {
        return FALSE;
    }

    hRes = DllOle32.pCoInitialize(NULL);
    if (!SUCCEEDED(hRes)) {
        return FALSE;
    }

    hRes = DllOle32.pCoCreateInstance(&CLSID_RadioManagement, NULL, CLSCTX_LOCAL_SERVER, &IID_IRadioManager, (void **)&RadioManager);
    if (!SUCCEEDED(hRes)) {
        return FALSE;
    }

    bRadioEnabled = TRUE;
    if (AirplaneModeEnabled) {
        bRadioEnabled = FALSE;
    }

    hRes = RadioManager->Vtbl->SetSystemRadioState(RadioManager, bRadioEnabled);
    if (!SUCCEEDED(hRes)) {
        Result = FALSE;
        goto Exit;
    }

    Result = TRUE;

Exit:

    if (RadioManager != NULL) {
        RadioManager->Vtbl->Release(RadioManager);
    }

    return Result;
}


// vim:sw=4:ts=4:et:
