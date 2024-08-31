/**
 * @file lib/dblclk.c
 *
 * Yori shell determine break characters for double click selection
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
 Return the set of characters that should be considered break characters when
 the user double clicks to select.  Break characters are never themselves
 selected.

 @param BreakChars On successful completion, populated with a set of
        characters that should be considered break characters.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGetSelectionDoubleClickBreakChars(
    __out PYORI_STRING BreakChars
    )
{
    YORI_ALLOC_SIZE_T WriteIndex;
    YORI_ALLOC_SIZE_T ReadIndex;
    YORI_STRING Substring;

    YoriLibInitEmptyString(BreakChars);
    if (!YoriLibAllocateAndGetEnvironmentVariable(_T("YORIQUICKEDITBREAKCHARS"), BreakChars) || BreakChars->LengthInChars == 0) {

        //
        //  0x2500 is Unicode full horizontal line (used by sdir)
        //  0x2502 is Unicode full vertical line (used by sdir)
        //  0x00BB is double angle quotation mark, used in elevated prompts
        //

        YoriLibConstantString(BreakChars, _T(" \t'[]<>|\x2500\x2502\x252c\x2534\x00BB"));
        return TRUE;
    }

    ReadIndex = 0;
    WriteIndex = 0;
    YoriLibInitEmptyString(&Substring);

    for (ReadIndex = 0; ReadIndex < BreakChars->LengthInChars; ReadIndex++) {
        if (ReadIndex + 1 < BreakChars->LengthInChars &&
            BreakChars->StartOfString[ReadIndex] == '0' &&
            BreakChars->StartOfString[ReadIndex + 1] == 'x') {

            YORI_ALLOC_SIZE_T CharsConsumed;
            YORI_MAX_SIGNED_T Number;

            Substring.StartOfString = &BreakChars->StartOfString[ReadIndex];
            Substring.LengthInChars = BreakChars->LengthInChars - ReadIndex;

            if (YoriLibStringToNumber(&Substring, FALSE, &Number, &CharsConsumed) &&
                CharsConsumed > 0 &&
                Number <= 0xFFFF) {

                BreakChars->StartOfString[WriteIndex] = (TCHAR)Number;
                WriteIndex++;
                ReadIndex = ReadIndex + CharsConsumed;
            }
        } else {
            if (ReadIndex != WriteIndex) {
                BreakChars->StartOfString[WriteIndex] = BreakChars->StartOfString[ReadIndex];
            }
            WriteIndex++;
        }
    }

    BreakChars->LengthInChars = WriteIndex;

    return TRUE;
}

/**
 Indicates if Yori QuickEdit should be enabled based on the state of the
 environment.  In this mode, the shell will disable QuickEdit support from
 the console and implement its own selection logic, but reenable QuickEdit
 for the benefit of applications.

 @return TRUE to indicate YoriQuickEdit should be enabled, and FALSE to
         leave the user's selection for QuickEdit behavior in effect.
 */
BOOL
YoriLibIsYoriQuickEditEnabled(VOID)
{
    YORI_STRING EnvVar;
    YORI_MAX_SIGNED_T llTemp;
    YORI_ALLOC_SIZE_T CharsConsumed;

    YoriLibInitEmptyString(&EnvVar);
    if (!YoriLibAllocateAndGetEnvironmentVariable(_T("YORIQUICKEDIT"), &EnvVar)) {
        return FALSE;
    }

    if (YoriLibStringToNumber(&EnvVar, TRUE, &llTemp, &CharsConsumed) && CharsConsumed > 0) {
        YoriLibFreeStringContents(&EnvVar);
        if (llTemp == 1) {
            return TRUE;
        }
    }
    YoriLibFreeStringContents(&EnvVar);
    return FALSE;
}


// vim:sw=4:ts=4:et:
