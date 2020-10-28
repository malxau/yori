/**
 * @file lib/numkey.c
 *
 * Yori numeric key code support routines
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
 For a key that may be a numeric key, progressively build a numeric key
 value.  The caller is expected to only invoke this routine if needed,
 meaning the Alt key is being held down.  It is invoked for each key press,
 but building a complete character scans multiple keypresses.

 @param NumericKeyValue Pointer to a key value to construct.  Initially
        this should be zero, but it is progressively updated with each key
        press.  The caller is expected to reset it to zero when the Alt
        key is released.

 @param NumericKeyType Pointer to the type of the numeric key which defines
        how it should be interpreted.  This routine can update this value
        based on the key that is pressed.

 @param KeyCode The key code for the key that is pressed.

 @param ScanCode The scan code for the key that is pressed.

 @return TRUE if this routine parsed the key and updated state.  FALSE if
         the key press is not meaningful to numeric key composition.
 */
BOOLEAN
YoriLibBuildNumericKey(
    __inout PDWORD NumericKeyValue,
    __inout PYORI_LIB_NUMERIC_KEY_TYPE NumericKeyType,
    __in DWORD KeyCode,
    __in DWORD ScanCode
    )
{
    DWORD Base;
    Base = 10;
    if (*NumericKeyType == YoriLibNumericKeyUnicode) {
        Base = 16;
    }
    if ((KeyCode == 'U' || KeyCode == 'u' || KeyCode == '+')
         && *NumericKeyType == YoriLibNumericKeyAscii) {

        *NumericKeyType = YoriLibNumericKeyUnicode;
    } else if (KeyCode >= '0' && KeyCode <= '9') {
        if (KeyCode == '0' && *NumericKeyValue == 0 && *NumericKeyType == YoriLibNumericKeyAscii) {
            *NumericKeyType = YoriLibNumericKeyAnsi;
        } else {
            *NumericKeyValue = *NumericKeyValue * Base + KeyCode - '0';
        }
    } else if (KeyCode >= VK_NUMPAD0 && KeyCode <= VK_NUMPAD9) {
        if (KeyCode == VK_NUMPAD0 && *NumericKeyValue == 0 && *NumericKeyType == YoriLibNumericKeyAscii) {
            *NumericKeyType = YoriLibNumericKeyAnsi;
        } else {
            *NumericKeyValue = *NumericKeyValue * Base + KeyCode - VK_NUMPAD0;
        }
    } else if (ScanCode >= 0x47 && ScanCode <= 0x49) {
        *NumericKeyValue = *NumericKeyValue * Base + ScanCode - 0x47 + 7;
    } else if (ScanCode >= 0x4b && ScanCode <= 0x4d) {
        *NumericKeyValue = *NumericKeyValue * Base + ScanCode - 0x4b + 4;
    } else if (ScanCode >= 0x4f && ScanCode <= 0x51) {
        *NumericKeyValue = *NumericKeyValue * Base + ScanCode - 0x4f + 1;
    } else if (ScanCode == 0x52) {
        if (*NumericKeyValue == 0 && *NumericKeyType == YoriLibNumericKeyAscii) {
            *NumericKeyType = YoriLibNumericKeyAnsi;
        } else {
            *NumericKeyValue = *NumericKeyValue * Base;
        }
    } else if (*NumericKeyType == YoriLibNumericKeyUnicode && KeyCode >= 'A' && KeyCode <= 'F') {
        *NumericKeyValue = *NumericKeyValue * Base + KeyCode - 'A' + 10;
    } else if (*NumericKeyType == YoriLibNumericKeyUnicode && KeyCode >= 'a' && KeyCode <= 'f') {
        *NumericKeyValue = *NumericKeyValue * Base + KeyCode - 'a' + 10;
    } else {
        return FALSE;
    }

    return TRUE;
}

/**
 MultiByteToWideChar seems to be able to convert the upper 128 characters
 from the OEM CP into Unicode correctly, but that leaves the low 32 characters
 which don't map to their Unicode equivalents.  This is a simple translation
 table for those characters.
 */
CONST TCHAR YoriLibLowAsciiToUnicodeTable[] = {
    0,      0x263a, 0x263b, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022,
    0x25d8, 0x25cb, 0x25d9, 0x2642, 0x2640, 0x266a, 0x266b, 0x263c,
    0x25ba, 0x25c4, 0x2195, 0x203c, 0x00b6, 0x00a7, 0x25ac, 0x21a8,
    0x2191, 0x2193, 0x2192, 0x2190, 0x221f, 0x2194, 0x25b2, 0x25bc
};

/**
 Translate a numeric key value and numeric key type into a character to
 apply.

 @param NumericKeyValue The numeric key value that is typically built from
        @ref YoriLibBuildNumericKey .

 @param NumericKeyType Specifies the type of the numeric key value.

 @param Char On successful completion, populated with the character.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriLibTranslateNumericKeyToChar(
    __in DWORD NumericKeyValue,
    __in YORI_LIB_NUMERIC_KEY_TYPE NumericKeyType,
    __out PTCHAR Char
    )
{
    UCHAR SmallKeyValue;
    TCHAR HostKeyValue[2];
    DWORD CodePage;

    SmallKeyValue = (UCHAR)NumericKeyValue;

    HostKeyValue[0] = HostKeyValue[1] = 0;

    if (NumericKeyType == YoriLibNumericKeyAscii && SmallKeyValue < 32) {
        HostKeyValue[0] = YoriLibLowAsciiToUnicodeTable[SmallKeyValue];
    } else if (NumericKeyType == YoriLibNumericKeyUnicode) {
        HostKeyValue[0] = (TCHAR)NumericKeyValue;
    } else {
        if (NumericKeyType == YoriLibNumericKeyAscii) {
            CodePage = CP_OEMCP;
        } else {
            YoriLibLoadUser32Functions();
            CodePage = CP_ACP;

            //
            //  GetKeyboardLayout requires NT4+.  By happy coincidence,
            //  that release also added support for LOCALE_RETURN_NUMBER
            //  which is a much cleaner interface.  Older releases get
            //  the active code page, which is generally correct unless
            //  the code page is configured for something different to
            //  the input language (eg. the code page is UTF8.)
            //

            if (DllUser32.pGetKeyboardLayout != NULL) {
                HKL KeyboardLayout;

                KeyboardLayout = DllUser32.pGetKeyboardLayout(0);

                GetLocaleInfo(MAKELCID(LOWORD((DWORD_PTR)KeyboardLayout), SORT_DEFAULT),
                              LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
                              (LPTSTR)&CodePage,
                              sizeof(CodePage));
            }
        }
        MultiByteToWideChar(CodePage,
                            0,
                            &SmallKeyValue,
                            1,
                            HostKeyValue,
                            1);
    }

    *Char = HostKeyValue[0];
    return TRUE;
}

// vim:sw=4:ts=4:et:
