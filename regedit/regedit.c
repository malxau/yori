/**
 * @file regedit/regedit.c
 *
 * Yori shell registry editor
 *
 * Copyright (c) 2019-2025 Malcolm J. Smith
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
#include <yoriwin.h>
#include <yoridlg.h>
#include "regedit.h"

/**
 Help text to display to the user.
 */
const
CHAR strRegeditHelpText[] =
        "\n"
        "Displays registry editor.\n"
        "\n"
        "REGEDIT [-license] [-a]\n"
        "\n"
        "   -a             Use ASCII characters for drawing\n";

/**
 The copyright year string to display with license text.
 */
const
TCHAR strRegeditCopyrightYear[] = _T("2022-2025");

/**
 Display usage text to the user.
 */
BOOL
RegeditHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Regedit %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strRegeditHelpText);
    return TRUE;
}

/**
 A structure describing the well known root keys.
 */
typedef struct _REGEDIT_KEY_NAME_PAIR {

    /**
     The string description of the root key name.
     */
    YORI_STRING KeyName;

    /**
     The pseudo handle to the root key.
     */
    HKEY KeyHandle;
} REGEDIT_KEY_NAME_PAIR, *PREGEDIT_KEY_NAME_PAIR;

/**
 An array of well known root keys.
 */
CONST REGEDIT_KEY_NAME_PAIR RegeditRootKeys[] = {
    {YORILIB_CONSTANT_STRING(_T("HKEY_CLASSES_ROOT")), HKEY_CLASSES_ROOT},
    {YORILIB_CONSTANT_STRING(_T("HKEY_CURRENT_USER")), HKEY_CURRENT_USER},
    {YORILIB_CONSTANT_STRING(_T("HKEY_LOCAL_MACHINE")), HKEY_LOCAL_MACHINE},
    {YORILIB_CONSTANT_STRING(_T("HKEY_USERS")), HKEY_USERS}
};

/**
 Display a dialog box containing the string form of a Win32 error code.

 @param Parent Pointer to the parent window.

 @param Error The Win32 error code.
 */
VOID
RegeditDisplayWin32Error(
    __in PYORI_WIN_CTRL_HANDLE Parent,
    __in DWORD Error
    )
{
    YORI_STRING Title;
    YORI_STRING Text;
    YORI_STRING ButtonText[1];
    LPTSTR ErrText;

    YoriLibConstantString(&Title, _T("Error"));
    YoriLibConstantString(&ButtonText[0], _T("&Ok"));

    ErrText = YoriLibGetWinErrorText(Error);
    YoriLibConstantString(&Text, ErrText);

    YoriDlgMessageBox(YoriWinGetWindowManagerHandle(YoriWinGetWindowFromWindowCtrl(Parent)),
                      &Title,
                      &Text,
                      1,
                      ButtonText,
                      0,
                      0);

    YoriLibFreeWinErrorText(ErrText);
}

/**
 Indicates that the selection within the key list has changed.

 @param Ctrl Pointer to the key list control.
 */
VOID
RegeditKeyListSelectionChanged(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PREGEDIT_CONTEXT RegeditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    RegeditContext = YoriWinGetControlContext(Parent);

    RegeditContext->MostRecentListSelectedControl = RegeditControlKeyList;
}

/**
 Indicates that the selection within the value list has changed.

 @param Ctrl Pointer to the value list control.
 */
VOID
RegeditValueListSelectionChanged(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PREGEDIT_CONTEXT RegeditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    RegeditContext = YoriWinGetControlContext(Parent);

    RegeditContext->MostRecentListSelectedControl = RegeditControlValueList;
}


/**
 Open the current registry key and populate the lists containing the subkeys
 and values within the currently active key.

 @param RegeditContext Pointer to the global registry context, indicating the
        currently active key.  This may be a root pseudohandle and subkey, or
        may be Depth 0, indicating that the well known root keys should be
        enumerated.

 @param KeyListCtrl Pointer to the list control containing subkeys.

 @param ValueListCtrl Pointer to the list control containing values.

 @param SelectKey Optionally points to a string containing the previously
        selected key.  On refresh the list of keys will change, but if
        supplied, either this key, or the one following it, or the last
        item in the list, should be selected.

 @param SelectValue Optionally points to a string containing the previously
        selected value.  On refresh the list of values will change, but if
        supplied, either this value, or the one following it, or the last
        item in the list, should be selected.
 */
VOID
RegeditPopulateKeyValueList(
    __in PREGEDIT_CONTEXT RegeditContext,
    __in PYORI_WIN_CTRL_HANDLE KeyListCtrl,
    __in PYORI_WIN_CTRL_HANDLE ValueListCtrl,
    __in_opt PYORI_STRING SelectKey,
    __in_opt PYORI_STRING SelectValue
    )
{
    YORI_ALLOC_SIZE_T Index;
    PYORI_WIN_CTRL_HANDLE Parent;
    YORI_ALLOC_SIZE_T ArrayCount;
    DWORD BytesRequired;
    YORI_ALLOC_SIZE_T ArrayElementSize;
    DWORD SubKeyCount;
    DWORD ValueCount;
    DWORD MaxValueNameLength;
    DWORD MaxSubKeyLength;
    PYORI_STRING StringArray;

    Parent = YoriWinGetControlParent(KeyListCtrl);
    YoriWinListClearAllItems(KeyListCtrl);
    YoriWinListClearAllItems(ValueListCtrl);

    if (RegeditContext->TreeDepth == 0) {
        for (Index = 0; Index < sizeof(RegeditRootKeys)/sizeof(RegeditRootKeys[0]); Index++) {
            YoriWinListAddItems(KeyListCtrl, &RegeditRootKeys[Index].KeyName, 1);
        }
    } else {
        YORI_STRING Text;
        DWORD Err;
        YORI_ALLOC_SIZE_T SelectIndex;
        DWORD ValueSize;
        DWORD MaxClassLength;
        DWORD MaxValueData;
        DWORD SecurityDescriptorLength;
        DWORD ClassLength;
        HKEY Key;
        FILETIME LastWriteTime;

        YoriLibConstantString(&Text, _T(".."));
        YoriWinListAddItems(KeyListCtrl, &Text, 1);

        Err = DllAdvApi32.pRegOpenKeyExW(RegeditContext->ActiveRootKey, RegeditContext->Subkey.StartOfString, 0, KEY_READ, &Key);
        if (Err != ERROR_SUCCESS) {
            RegeditDisplayWin32Error(Parent, Err);
            return;
        }

        ClassLength = 0;
        Err = DllAdvApi32.pRegQueryInfoKeyW(Key,
                                            NULL,
                                            &ClassLength,
                                            NULL,
                                            &SubKeyCount,
                                            &MaxSubKeyLength,
                                            &MaxClassLength,
                                            &ValueCount,
                                            &MaxValueNameLength,
                                            &MaxValueData,
                                            &SecurityDescriptorLength,
                                            &LastWriteTime);

        //
        //  Older versions of Windows insist all parameters are populated,
        //  including the class name, which we don't care about.  If needed,
        //  allocate space for it, call again, and throw it away.
        //

        if (Err == ERROR_MORE_DATA || Err == ERROR_INSUFFICIENT_BUFFER) {
            if (!YoriLibIsSizeAllocatable(ClassLength + 1)) {
                RegeditDisplayWin32Error(Parent, ERROR_OUTOFMEMORY);
                DllAdvApi32.pRegCloseKey(Key);
                return;
            }
            if (!YoriLibAllocateString(&Text, (YORI_ALLOC_SIZE_T)(ClassLength + 1))) {
                RegeditDisplayWin32Error(Parent, GetLastError());
                DllAdvApi32.pRegCloseKey(Key);
                return;
            }

            ClassLength = Text.LengthAllocated;
            Err = DllAdvApi32.pRegQueryInfoKeyW(Key,
                                                Text.StartOfString,
                                                &ClassLength,
                                                NULL,
                                                &SubKeyCount,
                                                &MaxSubKeyLength,
                                                &MaxClassLength,
                                                &ValueCount,
                                                &MaxValueNameLength,
                                                &MaxValueData,
                                                &SecurityDescriptorLength,
                                                &LastWriteTime);
            YoriLibFreeStringContents(&Text);
        }

        if (Err != ERROR_SUCCESS) {
            RegeditDisplayWin32Error(Parent, Err);
            DllAdvApi32.pRegCloseKey(Key);
            return;
        }

        //
        //  Use a single array both for keys and values.  This means the
        //  allocations need to be for the larger of the two.
        //

        if (!YoriLibIsSizeAllocatable(SubKeyCount) ||
            !YoriLibIsSizeAllocatable(ValueCount) ||
            !YoriLibIsSizeAllocatable(MaxSubKeyLength + 1) ||
            !YoriLibIsSizeAllocatable(MaxValueNameLength + 1)) {

            RegeditDisplayWin32Error(Parent, ERROR_NOT_ENOUGH_MEMORY);
            DllAdvApi32.pRegCloseKey(Key);
            return;
        }
        ArrayCount = (YORI_ALLOC_SIZE_T)SubKeyCount;
        if (ValueCount > ArrayCount) {
            ArrayCount = (YORI_ALLOC_SIZE_T)ValueCount;
        }

        ArrayElementSize = (YORI_ALLOC_SIZE_T)(MaxSubKeyLength + 1);
        if (MaxValueNameLength + 1 > ArrayElementSize) {
            ArrayElementSize = (YORI_ALLOC_SIZE_T)(MaxValueNameLength + 1);
        }

        BytesRequired = ArrayCount;
        BytesRequired = BytesRequired * sizeof(YORI_STRING);
        if (!YoriLibIsSizeAllocatable(BytesRequired)) {
            RegeditDisplayWin32Error(Parent, ERROR_NOT_ENOUGH_MEMORY);
            DllAdvApi32.pRegCloseKey(Key);
            return;
        }

        StringArray = YoriLibMalloc((YORI_ALLOC_SIZE_T)BytesRequired);
        if (StringArray == NULL) {
            RegeditDisplayWin32Error(Parent, GetLastError());
            DllAdvApi32.pRegCloseKey(Key);
            return;
        }

        for (Index = 0; Index < ArrayCount; Index++) {
            YoriLibInitEmptyString(&StringArray[Index]);
        }

        //
        //  Enumerate keys
        //

        for (Index = 0; Index < SubKeyCount; Index++) {
            if (!YoriLibAllocateString(&StringArray[Index], ArrayElementSize)) {
                Err = ERROR_OUTOFMEMORY;
                break;
            }
            ValueSize = ArrayElementSize;
            Err = DllAdvApi32.pRegEnumKeyExW(Key, Index, StringArray[Index].StartOfString, &ValueSize, NULL, NULL, NULL, &LastWriteTime);

            //
            //  MSFIX Possibly reallocate
            //

            if (Err == ERROR_NO_MORE_ITEMS) {
                Err = ERROR_SUCCESS;
                break;
            } else if (Err != ERROR_SUCCESS) {
                break;
            }

            if (!YoriLibIsSizeAllocatable(ValueSize)) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            StringArray[Index].LengthInChars = (YORI_ALLOC_SIZE_T)ValueSize;
        }

        //
        //  If it failed, tell the user.  Otherwise, sort, and add them to the
        //  list.  If an item should be selected, find the index of a matching
        //  string or the first item to be greater than it, and select that.
        //

        if (Err != ERROR_SUCCESS) {
            RegeditDisplayWin32Error(Parent, Err);
        } else if (Index > 0) {
            YoriLibSortStringArray(StringArray, Index);
            YoriWinListAddItems(KeyListCtrl, StringArray, Index);
            if (SelectKey != NULL) {
                for (SelectIndex = 0; SelectIndex < Index; SelectIndex++) {
                    if (YoriLibCompareStringIns(&StringArray[SelectIndex], SelectKey) >= 0) {
                        break;
                    }
                }

                //
                //  Because of the .. entry above, we normally move forward one
                //  element.  The exception is if we've already navigated
                //  beyond the final element.
                //

                if (SelectIndex < Index) {
                    SelectIndex++;
                }
                YoriWinListSetActiveOption(KeyListCtrl, SelectIndex);
            }
        }

        Err = ERROR_SUCCESS;

        //
        //  Collect values, sort, find item to select, etc.
        //

        for (Index = 0; Index < ValueCount; Index++) {
            if (StringArray[Index].LengthAllocated == 0) {
                if (!YoriLibAllocateString(&StringArray[Index], (YORI_ALLOC_SIZE_T)(MaxValueNameLength + 1))) {
                    Err = ERROR_OUTOFMEMORY;
                    break;
                }
            }
            ValueSize = StringArray[Index].LengthAllocated;
            Err = DllAdvApi32.pRegEnumValueW(Key, Index, StringArray[Index].StartOfString, &ValueSize, NULL, NULL, NULL, NULL);

            //
            //  MSFIX Possibly reallocate
            //
            if (Err == ERROR_NO_MORE_ITEMS) {
                Err = ERROR_SUCCESS;
                break;
            } else if (Err != ERROR_SUCCESS) {
                break;
            }

            if (!YoriLibIsSizeAllocatable(ValueSize)) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            StringArray[Index].LengthInChars = (YORI_ALLOC_SIZE_T)ValueSize;
        }

        if (Err != ERROR_SUCCESS) {
            RegeditDisplayWin32Error(Parent, Err);
        } else {
            YoriLibSortStringArray(StringArray, Index);
            YoriWinListAddItems(ValueListCtrl, StringArray, Index);
            if (SelectValue != NULL) {
                for (SelectIndex = 0; SelectIndex < Index; SelectIndex++) {
                    if (YoriLibCompareStringIns(&StringArray[SelectIndex], SelectValue) >= 0) {
                        break;
                    }
                }

                if (SelectIndex == Index) {
                    SelectIndex--;
                }
                YoriWinListSetActiveOption(ValueListCtrl, SelectIndex);
            }
        }

        for (Index = 0; Index < ArrayCount; Index++) {
            YoriLibFreeStringContents(&StringArray[Index]);
        }
        YoriLibFree(StringArray);

        DllAdvApi32.pRegCloseKey(Key);
    }
}

/**
 Display updated contents for the current list of keys and values within the
 key.

 @param RegeditContext Pointer to the regedit context which indicates the
        currently opened registry key.

 @param Parent Pointer to the parent window control.
 */
VOID
RegeditRefreshView(
    __in PREGEDIT_CONTEXT RegeditContext,
    __in PYORI_WIN_CTRL_HANDLE Parent
    )
{
    PYORI_WIN_CTRL_HANDLE KeyList;
    PYORI_WIN_CTRL_HANDLE ValueList;
    YORI_STRING PreviouslySelectedKey;
    YORI_STRING PreviouslySelectedValue;
    PYORI_STRING SelectKey;
    PYORI_STRING SelectValue;
    YORI_ALLOC_SIZE_T ActiveOption;

    //
    //  Find the controls.
    //

    KeyList = YoriWinFindControlById(Parent, RegeditControlKeyList);
    ASSERT(KeyList != NULL);
    __analysis_assume(KeyList != NULL);

    ValueList = YoriWinFindControlById(Parent, RegeditControlValueList);
    ASSERT(ValueList != NULL);
    __analysis_assume(ValueList != NULL);

    //
    //  See if any text is selected in each control, and if so, save it.
    //

    YoriLibInitEmptyString(&PreviouslySelectedKey);
    YoriLibInitEmptyString(&PreviouslySelectedValue);

    SelectKey = NULL;
    SelectValue = NULL;

    if (YoriWinListGetActiveOption(KeyList, &ActiveOption)) {
        if (YoriWinListGetItemText(KeyList, ActiveOption, &PreviouslySelectedKey)) {
            SelectKey = &PreviouslySelectedKey;
        }
    }

    if (YoriWinListGetActiveOption(ValueList, &ActiveOption)) {
        if (YoriWinListGetItemText(ValueList, ActiveOption, &PreviouslySelectedValue)) {
            SelectValue = &PreviouslySelectedValue;
        }
    }

    //
    //  Repopulate the lists from the top, and try to select either the
    //  currently selected text or something adjacent to it.
    //

    RegeditPopulateKeyValueList(RegeditContext, KeyList, ValueList, SelectKey, SelectValue);

    YoriLibFreeStringContents(&PreviouslySelectedKey);
    YoriLibFreeStringContents(&PreviouslySelectedValue);
}

/**
 Find the currently selected key within the key list control, and delete
 it.

 @param RegeditContext Pointer to the global registry editor context.

 @param Parent Pointer to the main window.

 @param KeyList Pointer to the list control containing keys.

 @param ValueList Pointer to the list control containing values.

 @param SelectedKeyIndex Indicates the currently selected item within the
        key list control.
 */
VOID
RegeditDeleteSelectedKey(
    __in PREGEDIT_CONTEXT RegeditContext,
    __in PYORI_WIN_CTRL_HANDLE Parent,
    __in PYORI_WIN_CTRL_HANDLE KeyList,
    __in PYORI_WIN_CTRL_HANDLE ValueList,
    __in YORI_ALLOC_SIZE_T SelectedKeyIndex
    )
{
    YORI_STRING KeyName;
    DWORD Err;
    HKEY Key;

    UNREFERENCED_PARAMETER(ValueList);

    YoriLibInitEmptyString(&KeyName);
    YoriWinListGetItemText(KeyList, SelectedKeyIndex, &KeyName);

    //
    //  MSFIX Windows regedit will recursively delete keys in this situation
    //

    Err = DllAdvApi32.pRegOpenKeyExW(RegeditContext->ActiveRootKey, RegeditContext->Subkey.StartOfString, 0, KEY_READ | KEY_SET_VALUE, &Key);
    if (Err != ERROR_SUCCESS) {
        YoriLibFreeStringContents(&KeyName);
        RegeditDisplayWin32Error(Parent, Err);
        return;
    }

    Err = DllAdvApi32.pRegDeleteKeyW(Key, KeyName.StartOfString);
    DllAdvApi32.pRegCloseKey(Key);
    if (Err != ERROR_SUCCESS) {
        YoriLibFreeStringContents(&KeyName);
        RegeditDisplayWin32Error(Parent, Err);
        return;
    }

    YoriLibFreeStringContents(&KeyName);

    RegeditRefreshView(RegeditContext, Parent);
}

/**
 Find the currently selected value within the value list control, and delete
 it.

 @param RegeditContext Pointer to the global registry editor context.

 @param Parent Pointer to the main window.

 @param KeyList Pointer to the list control containing keys.

 @param ValueList Pointer to the list control containing values.

 @param SelectedValueIndex Indicates the currently selected item within the
        value list control.
 */
VOID
RegeditDeleteSelectedValue(
    __in PREGEDIT_CONTEXT RegeditContext,
    __in PYORI_WIN_CTRL_HANDLE Parent,
    __in PYORI_WIN_CTRL_HANDLE KeyList,
    __in PYORI_WIN_CTRL_HANDLE ValueList,
    __in YORI_ALLOC_SIZE_T SelectedValueIndex
    )
{
    YORI_STRING ValueName;
    DWORD Err;
    HKEY Key;

    UNREFERENCED_PARAMETER(KeyList);

    YoriLibInitEmptyString(&ValueName);
    YoriWinListGetItemText(ValueList, SelectedValueIndex, &ValueName);

    Err = DllAdvApi32.pRegOpenKeyExW(RegeditContext->ActiveRootKey, RegeditContext->Subkey.StartOfString, 0, KEY_READ | KEY_SET_VALUE, &Key);
    if (Err != ERROR_SUCCESS) {
        YoriLibFreeStringContents(&ValueName);
        RegeditDisplayWin32Error(Parent, Err);
        return;
    }

    Err = DllAdvApi32.pRegDeleteValueW(Key, ValueName.StartOfString);
    DllAdvApi32.pRegCloseKey(Key);
    if (Err != ERROR_SUCCESS) {
        YoriLibFreeStringContents(&ValueName);
        RegeditDisplayWin32Error(Parent, Err);
        return;
    }

    YoriLibFreeStringContents(&ValueName);

    RegeditRefreshView(RegeditContext, Parent);
}

/**
 Find the currently selected value within the value list control, and open a
 child dialog to edit it.

 @param RegeditContext Pointer to the global registry editor context.

 @param Parent Pointer to the main window.

 @param KeyList Pointer to the list control containing keys.

 @param ValueList Pointer to the list control containing values.

 @param SelectedValueIndex Indicates the currently selected item within the
        value list control.
 */
VOID
RegeditEditSelectedValue(
    __in PREGEDIT_CONTEXT RegeditContext,
    __in PYORI_WIN_CTRL_HANDLE Parent,
    __in PYORI_WIN_CTRL_HANDLE KeyList,
    __in PYORI_WIN_CTRL_HANDLE ValueList,
    __in YORI_ALLOC_SIZE_T SelectedValueIndex
    )
{
    YORI_STRING OriginalValueName;
    YORI_STRING NewValueName;
    YORI_STRING Value;
    YORI_STRING Title;
    YORI_STRING ButtonText[1];
    DWORD Err;
    DWORD DataType;
    YORI_ALLOC_SIZE_T NativeDataSize;
    DWORD CharsRequired;
    DWORD DataSize;
    HKEY Key;
    PVOID DataPtr;
    BOOLEAN ReadOnly;

    UNREFERENCED_PARAMETER(KeyList);

    YoriLibInitEmptyString(&OriginalValueName);
    YoriLibInitEmptyString(&NewValueName);
    YoriWinListGetItemText(ValueList, SelectedValueIndex, &OriginalValueName);

    //
    //  If the key can't be opened with KEY_SET_VALUE, open it with KEY_READ
    //  only and display the value as read only
    //

    ReadOnly = FALSE;
    Err = DllAdvApi32.pRegOpenKeyExW(RegeditContext->ActiveRootKey, RegeditContext->Subkey.StartOfString, 0, KEY_READ | KEY_SET_VALUE, &Key);
    if (Err == ERROR_ACCESS_DENIED) {
        ReadOnly = TRUE;
        Err = DllAdvApi32.pRegOpenKeyExW(RegeditContext->ActiveRootKey, RegeditContext->Subkey.StartOfString, 0, KEY_READ, &Key);
    }
    if (Err != ERROR_SUCCESS) {
        YoriLibFreeStringContents(&OriginalValueName);
        RegeditDisplayWin32Error(Parent, Err);
        return;
    }

    DataPtr = NULL;
    YoriLibInitEmptyString(&Value);
    YoriLibConstantString(&ButtonText[0], _T("&Ok"));

    while(TRUE) {

        DataSize = Value.LengthAllocated * sizeof(TCHAR);
        Err = DllAdvApi32.pRegQueryValueExW(Key, OriginalValueName.StartOfString, NULL, &DataType, DataPtr, &DataSize);
        if (Err != ERROR_SUCCESS && Err != ERROR_MORE_DATA) {
            RegeditDisplayWin32Error(Parent, Err);
            DllAdvApi32.pRegCloseKey(Key);
            YoriLibFreeStringContents(&OriginalValueName);
            YoriLibFreeStringContents(&Value);
            return;
        }

        if (!YoriLibIsSizeAllocatable(DataSize)) {
            RegeditDisplayWin32Error(Parent, ERROR_NOT_ENOUGH_MEMORY);
            DllAdvApi32.pRegCloseKey(Key);
            YoriLibFreeStringContents(&OriginalValueName);
            YoriLibFreeStringContents(&Value);
            return;
        }

        if (DataType != REG_DWORD &&
            DataType != REG_QWORD &&
            DataType != REG_BINARY &&
            DataType != REG_SZ &&
            DataType != REG_EXPAND_SZ) {

            switch(DataType) {
                case REG_DWORD_BIG_ENDIAN:
                    YoriLibConstantString(&Title, _T("REG_DWORD_BIG_ENDIAN data type not supported"));
                    break;
                case REG_LINK:
                    YoriLibConstantString(&Title, _T("REG_LINK data type not supported"));
                    break;
                case REG_MULTI_SZ:
                    YoriLibConstantString(&Title, _T("REG_MULTI_SZ data type not supported"));
                    break;
                case REG_RESOURCE_LIST:
                    YoriLibConstantString(&Title, _T("REG_RESOURCE_LIST data type not supported"));
                    break;
                case REG_FULL_RESOURCE_DESCRIPTOR:
                    YoriLibConstantString(&Title, _T("REG_FULL_RESOURCE_DESCRIPTOR data type not supported"));
                    break;
                case REG_RESOURCE_REQUIREMENTS_LIST:
                    YoriLibConstantString(&Title, _T("REG_RESOURCE_REQUIREMENTS_LIST data type not supported"));
                    break;
                default:
                    YoriLibConstantString(&Title, _T("Unknown data type not supported"));
                    break;
            }

            YoriDlgMessageBox(YoriWinGetWindowManagerHandle(YoriWinGetWindowFromWindowCtrl(Parent)),
                              &Title,
                              &Title,
                              1,
                              ButtonText,
                              0,
                              0);

            break;
        }

        if (Err == ERROR_SUCCESS &&
            DataSize <= Value.LengthAllocated * sizeof(TCHAR)) {

            if (DataType == REG_SZ || DataType == REG_EXPAND_SZ) {
                Value.LengthInChars = (YORI_ALLOC_SIZE_T)(DataSize / sizeof(TCHAR));
                while (Value.LengthInChars > 0 &&
                       Value.StartOfString[Value.LengthInChars - 1] == '\0') {

                    Value.LengthInChars--;
                }

                YoriLibCloneString(&NewValueName, &OriginalValueName);

                if (RegeditEditStringValue(RegeditContext,
                                           YoriWinGetWindowManagerHandle(YoriWinGetWindowFromWindowCtrl(Parent)),
                                           &NewValueName,
                                           FALSE,
                                           &Value,
                                           ReadOnly)) {

                    ASSERT(YoriLibIsStringNullTerminated(&Value));
                    Value.LengthInChars++;

                    Err = DllAdvApi32.pRegSetValueExW(Key, NewValueName.StartOfString, 0, DataType, (LPBYTE)Value.StartOfString, Value.LengthInChars * sizeof(TCHAR));

                    if (Err != ERROR_SUCCESS) {
                        RegeditDisplayWin32Error(Parent, Err);
                    }
                }

            } else if (DataType == REG_DWORD) {
                DWORD Data;
                YORI_MAX_UNSIGNED_T LongData;
                LongData = 0;
                if (DataSize >= sizeof(DWORD)) {
                    Data = *(PDWORD)Value.StartOfString;
                    LongData = Data;
                }

                YoriLibCloneString(&NewValueName, &OriginalValueName);

                if (RegeditEditNumericValue(RegeditContext,
                                            YoriWinGetWindowManagerHandle(YoriWinGetWindowFromWindowCtrl(Parent)),
                                            &NewValueName,
                                            FALSE,
                                            &LongData,
                                            ReadOnly)) {

                    Data = (DWORD)LongData;
                    Err = DllAdvApi32.pRegSetValueExW(Key, NewValueName.StartOfString, 0, DataType, (LPBYTE)&Data, sizeof(DWORD));
                    if (Err != ERROR_SUCCESS) {
                        RegeditDisplayWin32Error(Parent, Err);
                    }
                }
            } else if (DataType == REG_QWORD) {
                YORI_MAX_UNSIGNED_T Data;
                Data = 0;
                if (DataSize >= sizeof(DWORDLONG)) {
                    Data = (YORI_MAX_UNSIGNED_T)(*(PDWORDLONG)Value.StartOfString);
                }

                YoriLibCloneString(&NewValueName, &OriginalValueName);

                if (RegeditEditNumericValue(RegeditContext,
                                            YoriWinGetWindowManagerHandle(YoriWinGetWindowFromWindowCtrl(Parent)),
                                            &NewValueName,
                                            FALSE,
                                            &Data,
                                            ReadOnly)) {

                    Err = DllAdvApi32.pRegSetValueExW(Key, NewValueName.StartOfString, 0, DataType, (LPBYTE)&Data, sizeof(DWORDLONG));
                    if (Err != ERROR_SUCCESS) {
                        RegeditDisplayWin32Error(Parent, Err);
                    }
                }
            } else if (DataType == REG_BINARY) {
                PUCHAR Data;
                Data = (PUCHAR)Value.StartOfString;
                NativeDataSize = (YORI_ALLOC_SIZE_T)DataSize;
                YoriLibCloneString(&NewValueName, &OriginalValueName);
                if (RegeditEditBinaryValue(RegeditContext,
                                           YoriWinGetWindowManagerHandle(YoriWinGetWindowFromWindowCtrl(Parent)),
                                           &NewValueName,
                                           FALSE,
                                           &Data,
                                           &NativeDataSize,
                                           ReadOnly)) {

                    //
                    //  The pointer may or may not have been modified, but
                    //  the new one has a reference and the old one doesn't,
                    //  so set the string to point to the referenced memory.
                    //

                    Value.MemoryToFree =
                    Value.StartOfString = (LPTSTR)Data;
                    DataSize = NativeDataSize;

                    Err = DllAdvApi32.pRegSetValueExW(Key, NewValueName.StartOfString, 0, DataType, (LPBYTE)Data, DataSize);
                    if (Err != ERROR_SUCCESS) {
                        RegeditDisplayWin32Error(Parent, Err);
                    }
                }
            }
            // MSFIX multi string

            if (YoriLibCompareString(&NewValueName, &OriginalValueName) != 0) {
                Err = DllAdvApi32.pRegDeleteValueW(Key, OriginalValueName.StartOfString);
                if (Err != ERROR_SUCCESS) {
                    RegeditDisplayWin32Error(Parent, Err);
                }
                RegeditRefreshView(RegeditContext, Parent);
            }
            break;
        }

        YoriLibFreeStringContents(&Value);

        //
        //  NewValueName should only be used once the value has been queried
        //  and is being modified, so no attempt is made to free it here
        //

        ASSERT(NewValueName.MemoryToFree == NULL);

        //
        //  Allocate buffer, rounding up to the next TCHAR, since the values
        //  here may be byte aligned
        //

        CharsRequired = (DataSize + sizeof(TCHAR) - 1) / sizeof(TCHAR);
        if (!YoriLibIsSizeAllocatable(CharsRequired)) {
            DllAdvApi32.pRegCloseKey(Key);
            YoriLibFreeStringContents(&OriginalValueName);
            return;
        }
        if (!YoriLibAllocateString(&Value, (YORI_ALLOC_SIZE_T)CharsRequired)) {
            DllAdvApi32.pRegCloseKey(Key);
            YoriLibFreeStringContents(&OriginalValueName);
            return;
        }
        DataPtr = Value.StartOfString;
    }

    DllAdvApi32.pRegCloseKey(Key);

    YoriLibFreeStringContents(&OriginalValueName);
    YoriLibFreeStringContents(&NewValueName);
    YoriLibFreeStringContents(&Value);

    YoriWinListSetActiveOption(ValueList, SelectedValueIndex);
}

/**
 Find the currently selected key within the key list control, and navigate to
 it.  This may be navigating to a subkey, or a parent key, or the magic root
 which is populated with predefined keys.`

 @param RegeditContext Pointer to the global registry editor context.

 @param Parent Pointer to the main window.

 @param KeyList Pointer to the list control containing keys.

 @param ValueList Pointer to the list control containing values.

 @param SelectedKeyIndex Indicates the currently selected item within the key
        list control.
 */
VOID
RegeditNavigateToSelectedKey(
    __in PREGEDIT_CONTEXT RegeditContext,
    __in PYORI_WIN_CTRL_HANDLE Parent,
    __in PYORI_WIN_CTRL_HANDLE KeyList,
    __in PYORI_WIN_CTRL_HANDLE ValueList,
    __in YORI_ALLOC_SIZE_T SelectedKeyIndex
    )
{
    PYORI_WIN_CTRL_HANDLE KeyCaption;
    YORI_STRING String;
    YORI_STRING NewCaption;

    KeyCaption = YoriWinFindControlById(Parent, RegeditControlKeyName);
    ASSERT(KeyCaption != NULL);
    __analysis_assume(KeyCaption != NULL);

    YoriLibInitEmptyString(&String);
    YoriWinListGetItemText(KeyList, SelectedKeyIndex, &String);

    if (YoriLibCompareStringLit(&String, _T("..")) == 0) {
        LPTSTR FinalSlash;
        FinalSlash = YoriLibFindRightMostCharacter(&RegeditContext->Subkey, '\\');
        if (FinalSlash != NULL) {
            RegeditContext->Subkey.LengthInChars = (YORI_ALLOC_SIZE_T)(FinalSlash - RegeditContext->Subkey.StartOfString);
        } else {
            RegeditContext->Subkey.LengthInChars = 0;
        }
        if (RegeditContext->Subkey.LengthAllocated != 0) {
            ASSERT(RegeditContext->Subkey.LengthAllocated > RegeditContext->Subkey.LengthInChars);
            RegeditContext->Subkey.StartOfString[RegeditContext->Subkey.LengthInChars] = '\0';
        }
        ASSERT(RegeditContext->TreeDepth > 0);
        RegeditContext->TreeDepth--;
    } else if (RegeditContext->TreeDepth == 0) {
        RegeditContext->ActiveRootKey = RegeditRootKeys[SelectedKeyIndex].KeyHandle;
        ASSERT(RegeditContext->Subkey.LengthInChars == 0);
        RegeditContext->TreeDepth++;
    } else {
        YORI_STRING CombinedString;
        if (!YoriLibAllocateString(&CombinedString, RegeditContext->Subkey.LengthInChars + 1 + String.LengthInChars + 1)) {
            YoriLibFreeStringContents(&String);
            return;
        }

        if (RegeditContext->TreeDepth == 1) {
            YoriLibYPrintf(&CombinedString, _T("%y"), &String);
        } else {
            YoriLibYPrintf(&CombinedString, _T("%y\\%y"), &RegeditContext->Subkey, &String);
        }

        YoriLibFreeStringContents(&RegeditContext->Subkey);
        YoriLibCloneString(&RegeditContext->Subkey, &CombinedString);
        YoriLibFreeStringContents(&CombinedString);
        RegeditContext->TreeDepth++;
    }
    YoriLibFreeStringContents(&String);

    YoriLibInitEmptyString(&NewCaption);
    if (RegeditContext->TreeDepth > 0) {
        PCYORI_STRING RootString;
        DWORD Index;

        RootString = NULL;
        for (Index = 0; Index < sizeof(RegeditRootKeys)/sizeof(RegeditRootKeys[0]); Index++) {
            if (RegeditContext->ActiveRootKey == RegeditRootKeys[Index].KeyHandle) {
                RootString = &RegeditRootKeys[Index].KeyName;
            }
        }

        ASSERT(RootString != NULL);

        if (YoriLibAllocateString(&String, RootString->LengthInChars + 1 + RegeditContext->Subkey.LengthInChars + 1)) {
            if (RegeditContext->TreeDepth == 1) {
                YoriLibYPrintf(&String, _T("%y"), RootString);
            } else {
                YoriLibYPrintf(&String, _T("%y\\%y"), RootString, &RegeditContext->Subkey);
            }

            YoriWinLabelSetCaption(KeyCaption, &String);
            YoriLibFreeStringContents(&String);
        }
    }

    RegeditPopulateKeyValueList(RegeditContext, KeyList, ValueList, NULL, NULL);
}

/**
 Callback invoked when the close button is clicked.  This terminates the
 registry editor application.

 @param Ctrl Pointer to the close button control.
 */
VOID
RegeditCloseButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    YoriWinCloseWindow(Parent, TRUE);
}

/**
 Callback invoked when the exit menu item is clicked.  This terminates the
 registry editor application.

 @param Ctrl Pointer to the menu control.
 */
VOID
RegeditExitButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    YoriWinCloseWindow(Parent, TRUE);
}

/**
 Callback invoked when the edit menu item is clicked.

 @param Ctrl Pointer to the menu control.
 */
VOID
RegeditEditButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE EditMenu;
    PYORI_WIN_CTRL_HANDLE NewMenu;
    PYORI_WIN_CTRL_HANDLE DeleteItem;

    PYORI_WIN_CTRL_HANDLE Parent;
    PREGEDIT_CONTEXT RegeditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    RegeditContext = YoriWinGetControlContext(Parent);

    EditMenu = YoriWinMenuBarGetSubmenuHandle(Ctrl, NULL, RegeditContext->EditMenuIndex);
    NewMenu = YoriWinMenuBarGetSubmenuHandle(Ctrl, EditMenu, RegeditContext->NewMenuIndex);
    DeleteItem = YoriWinMenuBarGetSubmenuHandle(Ctrl, EditMenu, RegeditContext->DeleteMenuIndex);

    if (RegeditContext->TreeDepth == 0) {
        YoriWinMenuBarDisableMenuItem(NewMenu);
        YoriWinMenuBarDisableMenuItem(DeleteItem);
    } else {
        YoriWinMenuBarEnableMenuItem(NewMenu);
        YoriWinMenuBarEnableMenuItem(DeleteItem);
    }

}

/**
 Invoked when the new key menu item is clicked.

 @param Ctrl Pointer to the menu control.
 */
VOID
RegeditNewKeyButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PREGEDIT_CONTEXT RegeditContext;
    YORI_STRING Title;
    YORI_STRING Value;
    DWORD Err;
    HKEY Key;
    HKEY NewKey;
    DWORD Disposition;

    Parent = YoriWinGetControlParent(Ctrl);
    YoriLibInitEmptyString(&Value);
    RegeditContext = YoriWinGetControlContext(Parent);

    //
    //  Can't create a key without a hive
    //

    if (RegeditContext->TreeDepth == 0) {
        return;
    }

    Err = DllAdvApi32.pRegOpenKeyExW(RegeditContext->ActiveRootKey, RegeditContext->Subkey.StartOfString, 0, KEY_READ | KEY_CREATE_SUB_KEY, &Key);
    if (Err != ERROR_SUCCESS) {
        RegeditDisplayWin32Error(Parent, Err);
        return;
    }

    YoriLibConstantString(&Title, _T("Create new key"));

    if (YoriDlgInput(YoriWinGetWindowManagerHandle(YoriWinGetWindowFromWindowCtrl(Parent)),
                     &Title,
                     FALSE,
                     &Value)) {

        ASSERT(YoriLibIsStringNullTerminated(&Value));
        Value.LengthInChars++;

        Err = DllAdvApi32.pRegCreateKeyExW(Key, Value.StartOfString, 0, NULL, 0, 0, NULL, &NewKey, &Disposition);
        if (Err != ERROR_SUCCESS) {
            RegeditDisplayWin32Error(Parent, Err);
        } else {
            DllAdvApi32.pRegCloseKey(NewKey);
        }

        YoriLibFreeStringContents(&Value);

        if (Disposition == REG_OPENED_EXISTING_KEY) {
            YORI_STRING ButtonText[1];

            YoriLibConstantString(&Title, _T("Key already exists"));
            YoriLibConstantString(&ButtonText[0], _T("&Ok"));

            YoriDlgMessageBox(YoriWinGetWindowManagerHandle(YoriWinGetWindowFromWindowCtrl(Parent)),
                              &Title,
                              &Title,
                              1,
                              ButtonText,
                              0,
                              0);
        }
    }

    DllAdvApi32.pRegCloseKey(Key);

    RegeditRefreshView(RegeditContext, Parent);
}

/**
 Invoked when the new string menu item or the new expandable string menu item
 is clicked.

 @param Ctrl Pointer to the menu control.

 @param DataType Indicates the registry data type to attach.
 */
VOID
RegeditNewStringWithType(
    __in PYORI_WIN_CTRL_HANDLE Ctrl,
    __in DWORD DataType
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PREGEDIT_CONTEXT RegeditContext;
    YORI_STRING ValueName;
    YORI_STRING Value;
    DWORD Err;
    HKEY Key;

    Parent = YoriWinGetControlParent(Ctrl);
    YoriLibInitEmptyString(&ValueName);
    YoriLibInitEmptyString(&Value);
    RegeditContext = YoriWinGetControlContext(Parent);

    //
    //  Can't create a value without a hive
    //

    if (RegeditContext->TreeDepth == 0) {
        return;
    }

    Err = DllAdvApi32.pRegOpenKeyExW(RegeditContext->ActiveRootKey, RegeditContext->Subkey.StartOfString, 0, KEY_READ | KEY_SET_VALUE, &Key);
    if (Err != ERROR_SUCCESS) {
        RegeditDisplayWin32Error(Parent, Err);
        return;
    }


    if (RegeditEditStringValue(RegeditContext,
                               YoriWinGetWindowManagerHandle(YoriWinGetWindowFromWindowCtrl(Parent)),
                               &ValueName,
                               FALSE,
                               &Value,
                               FALSE)) {

        ASSERT(YoriLibIsStringNullTerminated(&Value));
        Value.LengthInChars++;

        Err = DllAdvApi32.pRegSetValueExW(Key, ValueName.StartOfString, 0, DataType, (LPBYTE)Value.StartOfString, Value.LengthInChars * sizeof(TCHAR));
        if (Err != ERROR_SUCCESS) {
            RegeditDisplayWin32Error(Parent, Err);
        }

        YoriLibFreeStringContents(&ValueName);
        YoriLibFreeStringContents(&Value);
    }

    DllAdvApi32.pRegCloseKey(Key);

    RegeditRefreshView(RegeditContext, Parent);
}

/**
 Callback invoked when the new string menu item is clicked.

 @param Ctrl Pointer to the menu control.
 */
VOID
RegeditNewStringButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    RegeditNewStringWithType(Ctrl, REG_SZ);
}

/**
 Callback invoked when the new expandable string menu item is clicked.

 @param Ctrl Pointer to the menu control.
 */
VOID
RegeditNewExpandableStringButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    RegeditNewStringWithType(Ctrl, REG_EXPAND_SZ);
}

/**
 Callback invoked when the new DWORD menu item is clicked.

 @param Ctrl Pointer to the menu control.
 */
VOID
RegeditNewDWORDButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PREGEDIT_CONTEXT RegeditContext;
    YORI_STRING ValueName;
    DWORD Value;
    YORI_MAX_UNSIGNED_T LongValue;
    DWORD Err;
    HKEY Key;

    Parent = YoriWinGetControlParent(Ctrl);
    YoriLibInitEmptyString(&ValueName);
    RegeditContext = YoriWinGetControlContext(Parent);

    //
    //  Can't create a value without a hive
    //

    if (RegeditContext->TreeDepth == 0) {
        return;
    }

    Err = DllAdvApi32.pRegOpenKeyExW(RegeditContext->ActiveRootKey, RegeditContext->Subkey.StartOfString, 0, KEY_READ | KEY_SET_VALUE, &Key);
    if (Err != ERROR_SUCCESS) {
        RegeditDisplayWin32Error(Parent, Err);
        return;
    }

    LongValue = 0;

    if (RegeditEditNumericValue(RegeditContext,
                                YoriWinGetWindowManagerHandle(YoriWinGetWindowFromWindowCtrl(Parent)),
                                &ValueName,
                                FALSE,
                                &LongValue,
                                FALSE)) {

        Value = (DWORD)LongValue;

        Err = DllAdvApi32.pRegSetValueExW(Key, ValueName.StartOfString, 0, REG_DWORD, (LPBYTE)&Value, sizeof(DWORD));
        if (Err != ERROR_SUCCESS) {
            RegeditDisplayWin32Error(Parent, Err);
        }

        YoriLibFreeStringContents(&ValueName);
    }

    DllAdvApi32.pRegCloseKey(Key);

    RegeditRefreshView(RegeditContext, Parent);
}

/**
 Callback invoked when the new QWORD menu item is clicked.

 @param Ctrl Pointer to the menu control.
 */
VOID
RegeditNewQWORDButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PREGEDIT_CONTEXT RegeditContext;
    YORI_STRING ValueName;
    YORI_MAX_UNSIGNED_T Value;
    DWORD Err;
    HKEY Key;

    Parent = YoriWinGetControlParent(Ctrl);
    YoriLibInitEmptyString(&ValueName);
    RegeditContext = YoriWinGetControlContext(Parent);

    //
    //  Can't create a value without a hive
    //

    if (RegeditContext->TreeDepth == 0) {
        return;
    }

    Err = DllAdvApi32.pRegOpenKeyExW(RegeditContext->ActiveRootKey, RegeditContext->Subkey.StartOfString, 0, KEY_READ | KEY_SET_VALUE, &Key);
    if (Err != ERROR_SUCCESS) {
        RegeditDisplayWin32Error(Parent, Err);
        return;
    }

    Value = 0;

    if (RegeditEditNumericValue(RegeditContext,
                                YoriWinGetWindowManagerHandle(YoriWinGetWindowFromWindowCtrl(Parent)),
                                &ValueName,
                                FALSE,
                                &Value,
                                FALSE)) {

        Err = DllAdvApi32.pRegSetValueExW(Key, ValueName.StartOfString, 0, REG_QWORD, (LPBYTE)&Value, sizeof(DWORDLONG));
        if (Err != ERROR_SUCCESS) {
            RegeditDisplayWin32Error(Parent, Err);
        }

        YoriLibFreeStringContents(&ValueName);
    }

    DllAdvApi32.pRegCloseKey(Key);

    RegeditRefreshView(RegeditContext, Parent);
}

/**
 Callback invoked when the new binary menu item is clicked.

 @param Ctrl Pointer to the menu control.
 */
VOID
RegeditNewBinaryButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PREGEDIT_CONTEXT RegeditContext;
    YORI_STRING ValueName;
    PUCHAR Value;
    YORI_ALLOC_SIZE_T NativeValueLength;
    DWORD ValueLength;
    DWORD Err;
    HKEY Key;

    Parent = YoriWinGetControlParent(Ctrl);
    YoriLibInitEmptyString(&ValueName);
    RegeditContext = YoriWinGetControlContext(Parent);

    //
    //  Can't create a value without a hive
    //

    if (RegeditContext->TreeDepth == 0) {
        return;
    }

    Err = DllAdvApi32.pRegOpenKeyExW(RegeditContext->ActiveRootKey, RegeditContext->Subkey.StartOfString, 0, KEY_READ | KEY_SET_VALUE, &Key);
    if (Err != ERROR_SUCCESS) {
        RegeditDisplayWin32Error(Parent, Err);
        return;
    }

    Value = NULL;
    NativeValueLength = 0;

    if (RegeditEditBinaryValue(RegeditContext,
                               YoriWinGetWindowManagerHandle(YoriWinGetWindowFromWindowCtrl(Parent)),
                               &ValueName,
                               FALSE,
                               &Value,
                               &NativeValueLength,
                               FALSE)) {

        ValueLength = NativeValueLength;

        Err = DllAdvApi32.pRegSetValueExW(Key, ValueName.StartOfString, 0, REG_BINARY, (LPBYTE)Value, ValueLength);
        if (Err != ERROR_SUCCESS) {
            RegeditDisplayWin32Error(Parent, Err);
        }

        YoriLibFreeStringContents(&ValueName);
        if (Value != NULL) {
            YoriLibDereference(Value);
        }
    }

    DllAdvApi32.pRegCloseKey(Key);

    RegeditRefreshView(RegeditContext, Parent);
}

/**
 Callback invoked when the delete menu item is clicked.

 @param Ctrl Pointer to the menu control.
 */
VOID
RegeditDeleteButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    YORI_ALLOC_SIZE_T ActiveOption;
    PYORI_WIN_CTRL_HANDLE Parent;
    PREGEDIT_CONTEXT RegeditContext;
    PYORI_WIN_CTRL_HANDLE KeyList;
    PYORI_WIN_CTRL_HANDLE ValueList;

    Parent = YoriWinGetControlParent(Ctrl);
    RegeditContext = YoriWinGetControlContext(Parent);

    //
    //  Can't delete a value without a hive
    //

    if (RegeditContext->TreeDepth == 0) {
        return;
    }

    KeyList = YoriWinFindControlById(Parent, RegeditControlKeyList);
    ASSERT(KeyList != NULL);
    __analysis_assume(KeyList != NULL);

    ValueList = YoriWinFindControlById(Parent, RegeditControlValueList);
    ASSERT(ValueList != NULL);
    __analysis_assume(ValueList != NULL);

    if (RegeditContext->MostRecentListSelectedControl == RegeditControlKeyList) {
        if (YoriWinListGetActiveOption(KeyList, &ActiveOption)) {
            RegeditDeleteSelectedKey(RegeditContext, Parent, KeyList, ValueList, ActiveOption);
            return;
        }
    } else if (RegeditContext->MostRecentListSelectedControl == RegeditControlValueList) {

        if (YoriWinListGetActiveOption(ValueList, &ActiveOption)) {
            RegeditDeleteSelectedValue(RegeditContext, Parent, KeyList, ValueList, ActiveOption);
            return;
        }
    }
}

/**
 Callback invoked when the copy key menu item is clicked.

 @param Ctrl Pointer to the menu control.
 */
VOID
RegeditCopyKeyButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PREGEDIT_CONTEXT RegeditContext;
    PCYORI_STRING RootString;
    YORI_STRING String;
    DWORD Index;

    Parent = YoriWinGetControlParent(Ctrl);
    RegeditContext = YoriWinGetControlContext(Parent);

    if (RegeditContext->TreeDepth > 0) {

        RootString = NULL;
        for (Index = 0; Index < sizeof(RegeditRootKeys)/sizeof(RegeditRootKeys[0]); Index++) {
            if (RegeditContext->ActiveRootKey == RegeditRootKeys[Index].KeyHandle) {
                RootString = &RegeditRootKeys[Index].KeyName;
            }
        }

        ASSERT(RootString != NULL);

        if (YoriLibAllocateString(&String, RootString->LengthInChars + 1 + RegeditContext->Subkey.LengthInChars + 1)) {
            if (RegeditContext->TreeDepth == 1) {
                YoriLibYPrintf(&String, _T("%y"), RootString);
            } else {
                YoriLibYPrintf(&String, _T("%y\\%y"), RootString, &RegeditContext->Subkey);
            }

            YoriLibCopyTextWithProcessFallback(&String);
            YoriLibFreeStringContents(&String);
        }
    }
}


/**
 Callback invoked when the refresh menu item is clicked.

 @param Ctrl Pointer to the menu control.
 */
VOID
RegeditRefreshButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PREGEDIT_CONTEXT RegeditContext;

    Parent = YoriWinGetControlParent(Ctrl);
    RegeditContext = YoriWinGetControlContext(Parent);

    RegeditRefreshView(RegeditContext, Parent);
}

/**
 Callback invoked when the about menu item is clicked.  This displays a
 child dialog describing the application.

 @param Ctrl Pointer to the menu control.
 */
VOID
RegeditAboutButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    YORI_STRING Title;
    YORI_STRING Text;
    YORI_STRING LeftText;
    YORI_STRING CenteredText;
    YORI_STRING ButtonTexts[2];
    PYORI_WIN_WINDOW_HANDLE Parent;
    YORI_ALLOC_SIZE_T Index;
    DWORD ButtonClicked;

    Parent = YoriWinGetControlParent(Ctrl);

    YoriLibConstantString(&Title, _T("About"));
    YoriLibInitEmptyString(&Text);

    YoriLibYPrintf(&Text,
                   _T("Regedit %i.%02i\n")
#if YORI_BUILD_ID
                   _T("Build %i\n")
#endif
                   _T("%hs"),
                   YORI_VER_MAJOR,
                   YORI_VER_MINOR,
#if YORI_BUILD_ID
                   YORI_BUILD_ID,
#endif
                   strRegeditHelpText);

    if (Text.StartOfString == NULL) {
        return;
    }

    //
    //  Search through the combined string to find the split point where
    //  earlier text should be centered and later text should be left
    //  aligned.  This is done to allow documentation for switches to be
    //  legible.  The split point is therefore defined as the first place
    //  a newline is followed by a space, indicating documentation for a
    //  switch.
    //
    //  Note the label control will swallow all leading spaces in a line.
    //

    YoriLibInitEmptyString(&CenteredText);
    YoriLibInitEmptyString(&LeftText);

    for (Index = 0; Index < Text.LengthInChars; Index++) {
        if (Text.StartOfString[Index] == '\n' &&
            Index + 1 < Text.LengthInChars &&
            Text.StartOfString[Index + 1] == ' ') {

            CenteredText.StartOfString = Text.StartOfString;
            CenteredText.LengthInChars = Index;

            LeftText.StartOfString = &Text.StartOfString[Index + 1];
            LeftText.LengthInChars = Text.LengthInChars - Index - 1;

            break;
        }
    }

    YoriLibConstantString(&ButtonTexts[0], _T("&Ok"));
    YoriLibConstantString(&ButtonTexts[1], _T("&View License..."));

    ButtonClicked = YoriDlgAbout(YoriWinGetWindowManagerHandle(Parent),
                                 &Title,
                                 &CenteredText,
                                 &LeftText,
                                 2,
                                 ButtonTexts,
                                 0,
                                 0);

    YoriLibFreeStringContents(&Text);

    if (ButtonClicked == 2) {
        if (YoriLibMitLicenseText(strRegeditCopyrightYear, &Text)) {

            YoriLibInitEmptyString(&CenteredText);
            YoriLibConstantString(&Title, _T("License"));

            //
            //  Replace all single line breaks with spaces but leave one line
            //  break in the case of double line (paragraph) breaks.  The
            //  label control can decide how to format lines.
            //

            for (Index = 0; Index < Text.LengthInChars; Index++) {
                if (Text.StartOfString[Index] == '\n' &&
                    Index + 1 < Text.LengthInChars &&
                    Text.StartOfString[Index + 1] != '\n') {

                    Text.StartOfString[Index] = ' ';
                }
            }

            YoriDlgAbout(YoriWinGetWindowManagerHandle(Parent),
                         &Title,
                         &CenteredText,
                         &Text,
                         1,
                         ButtonTexts,
                         0,
                         0);
            YoriLibFreeStringContents(&Text);
        }
    }
}

/**
 Heuristically determine if the user is active in the key list or the value
 list, and either navigate to a new key or edit the selected value.

 @param Ctrl Pointer to the Go button control.
 */
VOID
RegeditGoButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    YORI_ALLOC_SIZE_T ActiveOption;
    PYORI_WIN_CTRL_HANDLE Parent;
    PREGEDIT_CONTEXT RegeditContext;
    PYORI_WIN_CTRL_HANDLE KeyList;
    PYORI_WIN_CTRL_HANDLE ValueList;

    Parent = YoriWinGetControlParent(Ctrl);
    RegeditContext = YoriWinGetControlContext(Parent);

    KeyList = YoriWinFindControlById(Parent, RegeditControlKeyList);
    ASSERT(KeyList != NULL);
    __analysis_assume(KeyList != NULL);

    ValueList = YoriWinFindControlById(Parent, RegeditControlValueList);
    ASSERT(ValueList != NULL);
    __analysis_assume(ValueList != NULL);

    if (RegeditContext->MostRecentListSelectedControl == RegeditControlKeyList) {
        if (YoriWinListGetActiveOption(KeyList, &ActiveOption)) {
            RegeditNavigateToSelectedKey(RegeditContext, Parent, KeyList, ValueList, ActiveOption);
            return;
        }
    } else if (RegeditContext->MostRecentListSelectedControl == RegeditControlValueList) {
        if (YoriWinListGetActiveOption(ValueList, &ActiveOption)) {
            RegeditEditSelectedValue(RegeditContext, Parent, KeyList, ValueList, ActiveOption);
            return;
        }
    }
}

/**
 Create the menu bar and add initial items to it.

 @param RegeditContext Pointer to the global registry editor context.

 @param Parent Handle to the main window.

 @return Pointer to the menu bar control if it was successfully created
         and populated, or NULL on failure.
 */
PYORI_WIN_CTRL_HANDLE
RegeditPopulateMenuBar(
    __in PREGEDIT_CONTEXT RegeditContext,
    __in PYORI_WIN_WINDOW_HANDLE Parent
    )
{
    YORI_WIN_MENU_ENTRY FileMenuEntries[1];
    YORI_WIN_MENU_ENTRY EditMenuEntries[5];
    YORI_WIN_MENU_ENTRY ViewMenuEntries[1];
    YORI_WIN_MENU_ENTRY NewMenuEntries[7];
    YORI_WIN_MENU_ENTRY HelpMenuEntries[1];
    YORI_WIN_MENU_ENTRY MenuEntries[4];
    YORI_WIN_MENU MenuBarItems;
    PYORI_WIN_CTRL_HANDLE Ctrl;
    YORI_ALLOC_SIZE_T MenuIndex;

    ZeroMemory(&NewMenuEntries, sizeof(NewMenuEntries));
    MenuIndex = 0;
    YoriLibConstantString(&NewMenuEntries[MenuIndex].Caption, _T("&Key"));
    NewMenuEntries[MenuIndex].NotifyCallback = RegeditNewKeyButtonClicked;
    MenuIndex++;
    NewMenuEntries[MenuIndex].Flags = YORI_WIN_MENU_ENTRY_SEPERATOR;
    MenuIndex++;
    YoriLibConstantString(&NewMenuEntries[MenuIndex].Caption, _T("&String Value"));
    NewMenuEntries[MenuIndex].NotifyCallback = RegeditNewStringButtonClicked;
    MenuIndex++;
    YoriLibConstantString(&NewMenuEntries[MenuIndex].Caption, _T("&DWORD Value"));
    NewMenuEntries[MenuIndex].NotifyCallback = RegeditNewDWORDButtonClicked;
    MenuIndex++;
    YoriLibConstantString(&NewMenuEntries[MenuIndex].Caption, _T("&QWORD Value"));
    NewMenuEntries[MenuIndex].NotifyCallback = RegeditNewQWORDButtonClicked;
    MenuIndex++;
    YoriLibConstantString(&NewMenuEntries[MenuIndex].Caption, _T("Expandable string Value"));
    NewMenuEntries[MenuIndex].NotifyCallback = RegeditNewExpandableStringButtonClicked;
    MenuIndex++;
    YoriLibConstantString(&NewMenuEntries[MenuIndex].Caption, _T("&Binary Value"));
    NewMenuEntries[MenuIndex].NotifyCallback = RegeditNewBinaryButtonClicked;
    // MSFIX multi string

    ZeroMemory(&FileMenuEntries, sizeof(FileMenuEntries));
    MenuIndex = 0;
    YoriLibConstantString(&FileMenuEntries[MenuIndex].Caption, _T("E&xit"));
    FileMenuEntries[MenuIndex].NotifyCallback = RegeditExitButtonClicked;

    ZeroMemory(&EditMenuEntries, sizeof(EditMenuEntries));
    MenuIndex = 0;
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Caption, _T("&New"));
    EditMenuEntries[MenuIndex].ChildMenu.ItemCount = sizeof(NewMenuEntries)/sizeof(NewMenuEntries[0]);
    EditMenuEntries[MenuIndex].ChildMenu.Items = NewMenuEntries;
    RegeditContext->NewMenuIndex = MenuIndex;

    MenuIndex++;
    EditMenuEntries[MenuIndex].Flags = YORI_WIN_MENU_ENTRY_SEPERATOR;
    MenuIndex++;
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Caption, _T("&Delete"));
    EditMenuEntries[MenuIndex].NotifyCallback = RegeditDeleteButtonClicked;
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Hotkey, _T("Del"));
    RegeditContext->DeleteMenuIndex = MenuIndex;

    MenuIndex++;
    EditMenuEntries[MenuIndex].Flags = YORI_WIN_MENU_ENTRY_SEPERATOR;
    MenuIndex++;
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Caption, _T("&Copy Key Name"));
    EditMenuEntries[MenuIndex].NotifyCallback = RegeditCopyKeyButtonClicked;
    YoriLibConstantString(&EditMenuEntries[MenuIndex].Hotkey, _T("Ctrl+C"));
    RegeditContext->CopyKeyMenuIndex = MenuIndex;

    ZeroMemory(&ViewMenuEntries, sizeof(ViewMenuEntries));
    MenuIndex = 0;
    YoriLibConstantString(&ViewMenuEntries[MenuIndex].Caption, _T("&Refresh"));
    YoriLibConstantString(&ViewMenuEntries[MenuIndex].Hotkey, _T("F5"));
    ViewMenuEntries[MenuIndex].NotifyCallback = RegeditRefreshButtonClicked;

    // MSFIX Copy current key

    ZeroMemory(&HelpMenuEntries, sizeof(HelpMenuEntries));
    MenuIndex = 0;
    YoriLibConstantString(&HelpMenuEntries[MenuIndex].Caption, _T("&About..."));
    HelpMenuEntries[MenuIndex].NotifyCallback = RegeditAboutButtonClicked;

    MenuBarItems.Items = MenuEntries;

    MenuIndex = 0;
    ZeroMemory(MenuEntries, sizeof(MenuEntries));
    YoriLibConstantString(&MenuEntries[MenuIndex].Caption, _T("&File"));
    MenuEntries[MenuIndex].ChildMenu.ItemCount = sizeof(FileMenuEntries)/sizeof(FileMenuEntries[0]);
    MenuEntries[MenuIndex].ChildMenu.Items = FileMenuEntries;

    MenuIndex++;
    YoriLibConstantString(&MenuEntries[MenuIndex].Caption, _T("&Edit"));
    MenuEntries[MenuIndex].NotifyCallback = RegeditEditButtonClicked;
    MenuEntries[MenuIndex].ChildMenu.ItemCount = sizeof(EditMenuEntries)/sizeof(EditMenuEntries[0]);
    MenuEntries[MenuIndex].ChildMenu.Items = EditMenuEntries;
    RegeditContext->EditMenuIndex = MenuIndex;

    MenuIndex++;
    YoriLibConstantString(&MenuEntries[MenuIndex].Caption, _T("&View"));
    MenuEntries[MenuIndex].ChildMenu.ItemCount = sizeof(ViewMenuEntries)/sizeof(ViewMenuEntries[0]);
    MenuEntries[MenuIndex].ChildMenu.Items = ViewMenuEntries;

    MenuIndex++;
    YoriLibConstantString(&MenuEntries[MenuIndex].Caption, _T("&Help"));
    MenuEntries[MenuIndex].ChildMenu.ItemCount = sizeof(HelpMenuEntries)/sizeof(HelpMenuEntries[0]);
    MenuEntries[MenuIndex].ChildMenu.Items = HelpMenuEntries;

    MenuBarItems.ItemCount = MenuIndex + 1;

    Ctrl = YoriWinMenuBarCreate(Parent, 0);
    if (Ctrl == NULL) {
        return NULL;
    }

    if (!YoriWinMenuBarAppendItems(Ctrl, &MenuBarItems)) {
        return NULL;
    }

    return Ctrl;
}

/**
 The minimum width in characters where regedit can hope to function.
 */
#define REGEDIT_MINIMUM_WIDTH (60)

/**
 The minimum height in characters where regedit can hope to function.
 */
#define REGEDIT_MINIMUM_HEIGHT (20)

/**
 A callback that is invoked when the window manager is being resized.  This
 typically means the user resized the window.  Since the purpose of regedit
 is to fully occupy the window space, this implies the main window needs to
 be repositioned and/or resized, and the controls within it need to be
 repositioned and/or resized to full the window.

 @param WindowHandle Handle to the main window.

 @param OldPosition The old dimensions of the window manager.

 @param NewPosition The new dimensions of the window manager.
 */
VOID
RegeditResizeWindowManager(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle,
    __in PSMALL_RECT OldPosition,
    __in PSMALL_RECT NewPosition
    )
{
    PREGEDIT_CONTEXT RegeditContext;
    PYORI_WIN_CTRL_HANDLE WindowCtrl;
    PYORI_WIN_CTRL_HANDLE Ctrl;
    SMALL_RECT Area;
    COORD NewSize;

    UNREFERENCED_PARAMETER(OldPosition);

    WindowCtrl = YoriWinGetCtrlFromWindow(WindowHandle);
    RegeditContext = YoriWinGetControlContext(WindowCtrl);

    NewSize.X = (SHORT)(NewPosition->Right - NewPosition->Left + 1);
    NewSize.Y = (SHORT)(NewPosition->Bottom - NewPosition->Top + 1);

    if (NewSize.X < REGEDIT_MINIMUM_WIDTH || NewSize.Y < REGEDIT_MINIMUM_HEIGHT) {
        return;
    }

    //
    //  Resize the main window, including capturing its new background
    //

    if (!YoriWinWindowReposition(WindowHandle, NewPosition)) {
        return;
    }

    //
    //  Reposition and resize child controls on the main window, causing them
    //  to redraw themselves
    //

    Area.Left = 0;
    Area.Top = 0;
    Area.Right = (SHORT)(NewSize.X - 1);
    Area.Bottom = 0;

    Ctrl = YoriWinFindControlById(WindowCtrl, RegeditControlMenuBar);
    ASSERT(Ctrl != NULL);
    __analysis_assume(Ctrl != NULL);

    YoriWinMenuBarReposition(Ctrl, &Area);

    YoriWinGetClientSize(WindowHandle, &NewSize);

    Area.Left = 1;
    Area.Top = 1;
    Area.Right = (WORD)(NewSize.X - 2);
    Area.Bottom = 1;

    Ctrl = YoriWinFindControlById(WindowCtrl, RegeditControlKeyName);
    ASSERT(Ctrl != NULL);
    __analysis_assume(Ctrl != NULL);

    YoriWinLabelReposition(Ctrl, &Area);

    Area.Top = 2;
    Area.Bottom = Area.Top;
    Area.Left = 3;
    Area.Right = (WORD)(Area.Left + sizeof("Keys:") - 1);

    Ctrl = YoriWinFindControlById(WindowCtrl, RegeditControlKeyCaption);
    ASSERT(Ctrl != NULL);
    __analysis_assume(Ctrl != NULL);

    YoriWinLabelReposition(Ctrl, &Area);

    Area.Top = 3;
    Area.Left = 1;
    Area.Bottom = (WORD)(NewSize.Y - 4);
    Area.Right = (WORD)(NewSize.X / 2 - 1);

    Ctrl = YoriWinFindControlById(WindowCtrl, RegeditControlKeyList);
    ASSERT(Ctrl != NULL);
    __analysis_assume(Ctrl != NULL);

    YoriWinListReposition(Ctrl, &Area);

    Area.Top = (WORD)(Area.Top - 1);
    Area.Left = (WORD)(Area.Right + 4);
    Area.Right = (WORD)(Area.Left + sizeof("Values:") - 1);
    Area.Bottom = Area.Top;

    Ctrl = YoriWinFindControlById(WindowCtrl, RegeditControlValueCaption);
    ASSERT(Ctrl != NULL);
    __analysis_assume(Ctrl != NULL);

    YoriWinLabelReposition(Ctrl, &Area);

    Area.Top = (WORD)(Area.Bottom + 1);
    Area.Left = (WORD)(Area.Left - 2);
    Area.Bottom = (WORD)(NewSize.Y - 4);
    Area.Right = (WORD)(NewSize.X - 2);

    Ctrl = YoriWinFindControlById(WindowCtrl, RegeditControlValueList);
    ASSERT(Ctrl != NULL);
    __analysis_assume(Ctrl != NULL);

    YoriWinListReposition(Ctrl, &Area);

    Area.Top = (WORD)(NewSize.Y - 3);
    Area.Bottom = (SHORT)(Area.Top + 2);
    Area.Left = (WORD)(1);
    Area.Right = (WORD)(Area.Left + 1 + sizeof("Close") - 1 + 2);

    Ctrl = YoriWinFindControlById(WindowCtrl, RegeditControlCloseButton);
    ASSERT(Ctrl != NULL);
    __analysis_assume(Ctrl != NULL);

    YoriWinButtonReposition(Ctrl, &Area);

    Area.Left = (WORD)(Area.Right + 2);
    Area.Right = (WORD)(Area.Left + 1 + sizeof("Go") - 1 + 2);

    Ctrl = YoriWinFindControlById(WindowCtrl, RegeditControlGoButton);
    ASSERT(Ctrl != NULL);
    __analysis_assume(Ctrl != NULL);

    YoriWinButtonReposition(Ctrl, &Area);
}


/**
 Display a registry editor window.

 @param RegeditContext Pointer to the global registry editor context.

 @return TRUE to indicate that the window was successfully created.  FALSE
         to indicate a failure or the user cancelling the operation.
 */
__success(return)
BOOLEAN
RegeditCreateMainWindow(
    __in PREGEDIT_CONTEXT RegeditContext
    )
{
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgr;
    PYORI_WIN_WINDOW_HANDLE Parent;
    COORD WindowSize;
    SMALL_RECT Area;
    YORI_STRING Caption;
    PYORI_WIN_CTRL_HANDLE KeyList;
    PYORI_WIN_CTRL_HANDLE ValueList;
    PYORI_WIN_CTRL_HANDLE Ctrl;
    DWORD_PTR Result;
    COORD WinMgrSize;

    if (!YoriWinOpenWindowManager(TRUE, YoriWinColorTableDefault, &WinMgr)) {
        return FALSE;
    }

    if (RegeditContext->UseAsciiDrawing) {
        YoriWinMgrSetAsciiDrawing(WinMgr, RegeditContext->UseAsciiDrawing);
    }

    if (!YoriWinGetWinMgrDimensions(WinMgr, &WinMgrSize)) {
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    if (WinMgrSize.X < 60 || WinMgrSize.Y < 20) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("regedit: window size too small\n"));
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    if (!YoriWinCreateWindow(WinMgr, WinMgrSize.X, WinMgrSize.Y, WinMgrSize.X, WinMgrSize.Y, 0, NULL, &Parent)) {
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriWinSetControlContext(Parent, RegeditContext);

    Ctrl = RegeditPopulateMenuBar(RegeditContext, Parent);
    YoriWinSetControlId(Ctrl, RegeditControlMenuBar);

    YoriWinGetClientSize(Parent, &WindowSize);

    Area.Left = 1;
    Area.Top = 1;
    Area.Right = (WORD)(WindowSize.X - 2);
    Area.Bottom = 1;

    YoriLibConstantString(&Caption, _T(""));

    Ctrl = YoriWinLabelCreate(Parent, &Area, &Caption, YORI_WIN_LABEL_NO_ACCELERATOR);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriWinSetControlId(Ctrl, RegeditControlKeyName);

    YoriLibConstantString(&Caption, _T("&Keys:"));

    Area.Top = 2;
    Area.Bottom = Area.Top;
    Area.Left = 3;
    Area.Right = (WORD)(Area.Left + Caption.LengthInChars - 1);

    Ctrl = YoriWinLabelCreate(Parent, &Area, &Caption, 0);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriWinSetControlId(Ctrl, RegeditControlKeyCaption);

    Area.Top = 3;
    Area.Left = 1;
    Area.Bottom = (WORD)(WindowSize.Y - 4);
    Area.Right = (WORD)(WindowSize.X / 2 - 1);

    KeyList = YoriWinListCreate(Parent, &Area, YORI_WIN_LIST_STYLE_VSCROLLBAR | YORI_WIN_LIST_STYLE_AUTO_HSCROLLBAR);
    if (KeyList == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriWinSetControlId(KeyList, RegeditControlKeyList);
    YoriWinListSetSelectionNotifyCallback(KeyList, RegeditKeyListSelectionChanged);

    YoriLibConstantString(&Caption, _T("&Values:"));

    Area.Top = (WORD)(Area.Top - 1);
    Area.Left = (WORD)(Area.Right + 4);
    Area.Right = (WORD)(Area.Left + Caption.LengthInChars - 1);
    Area.Bottom = Area.Top;

    Ctrl = YoriWinLabelCreate(Parent, &Area, &Caption, 0);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriWinSetControlId(Ctrl, RegeditControlValueCaption);

    Area.Top = (WORD)(Area.Bottom + 1);
    Area.Left = (WORD)(Area.Left - 2);
    Area.Bottom = (WORD)(WindowSize.Y - 4);
    Area.Right = (WORD)(WindowSize.X - 2);

    ValueList = YoriWinListCreate(Parent, &Area, YORI_WIN_LIST_STYLE_VSCROLLBAR | YORI_WIN_LIST_STYLE_AUTO_HSCROLLBAR);
    if (ValueList == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriWinSetControlId(ValueList, RegeditControlValueList);
    YoriWinListSetSelectionNotifyCallback(ValueList, RegeditValueListSelectionChanged);

    RegeditPopulateKeyValueList(RegeditContext, KeyList, ValueList, NULL, NULL);

    Area.Top = (SHORT)(WindowSize.Y - 3);
    Area.Bottom = (SHORT)(Area.Top + 2);

    YoriLibConstantString(&Caption, _T("&Close"));

    Area.Left = (SHORT)(1);
    Area.Right = (WORD)(Area.Left + 1 + Caption.LengthInChars - 1 + 2);

    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, YORI_WIN_BUTTON_STYLE_CANCEL, RegeditCloseButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriWinSetControlId(Ctrl, RegeditControlCloseButton);

    YoriLibConstantString(&Caption, _T("&Go"));

    Area.Left = (WORD)(Area.Right + 2);
    Area.Right = (WORD)(Area.Left + 1 + Caption.LengthInChars - 1 + 2);

    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, YORI_WIN_BUTTON_STYLE_DEFAULT, RegeditGoButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriWinSetControlId(Ctrl, RegeditControlGoButton);

    YoriWinSetWindowManagerResizeNotifyCallback(Parent, RegeditResizeWindowManager);

    Result = FALSE;
    if (!YoriWinProcessInputForWindow(Parent, &Result)) {
        Result = FALSE;
    }

    YoriWinDestroyWindow(Parent);
    YoriWinCloseWindowManager(WinMgr);
    return (BOOLEAN)Result;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the regedit builtin command.
 */
#define ENTRYPOINT YoriCmd_YREGEDIT
#else
/**
 The main entrypoint for the regedit standalone application.
 */
#define ENTRYPOINT ymain
#endif


/**
 Display yori shell registry editor.

 @param ArgC The number of arguments.

 @param ArgV The argument array.

 @return ExitCode, zero for success, nonzero for failure.
 */
DWORD
ENTRYPOINT(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOLEAN ArgumentUnderstood;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 0;
    YORI_STRING Arg;
    REGEDIT_CONTEXT RegeditContext;

    RegeditContext.UseAsciiDrawing = FALSE;
    RegeditContext.TreeDepth = 0;
    YoriLibInitEmptyString(&RegeditContext.Subkey);


    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                RegeditHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(strRegeditCopyrightYear);
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("a")) == 0) {
                RegeditContext.UseAsciiDrawing = TRUE;
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

    if (DllAdvApi32.pRegCloseKey == NULL ||
        DllAdvApi32.pRegCreateKeyExW == NULL ||
        DllAdvApi32.pRegDeleteKeyW == NULL ||
        DllAdvApi32.pRegDeleteValueW == NULL ||
        DllAdvApi32.pRegEnumKeyExW == NULL ||
        DllAdvApi32.pRegEnumValueW == NULL ||
        DllAdvApi32.pRegQueryInfoKeyW == NULL ||
        DllAdvApi32.pRegQueryValueExW == NULL ||
        DllAdvApi32.pRegSetValueExW == NULL ||
        DllAdvApi32.pRegOpenKeyExW == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("regedit: OS support not present\n"));
        return EXIT_FAILURE;
    }

    if (!RegeditCreateMainWindow(&RegeditContext)) {
        YoriLibFreeStringContents(&RegeditContext.Subkey);
        return EXIT_FAILURE;
    }
    YoriLibFreeStringContents(&RegeditContext.Subkey);
    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
