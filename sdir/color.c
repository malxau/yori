/**
 * @file sdir/color.c
 *
 * Colorful, sorted and optionally rich directory enumeration
 * for Windows.
 *
 * This module implements string parsing and rule application to select a
 * given set of color attributes to render any particular file with.
 *
 * Copyright (c) 2014-2017 Malcolm J. Smith
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

#include "sdir.h"

/**
 The default color to use when nothing else matches.  This is a little
 circular.  Default contains both a color and an instruction to use the
 previous window color.  If we're able to get a window color, it'll pick
 that, but if not, the previous window color is populated from the color
 here.
 */
YORILIB_COLOR_ATTRIBUTES SdirDefaultColor = {SDIR_ATTRCTRL_WINDOW_BG | SDIR_ATTRCTRL_WINDOW_FG, SDIR_DEFAULT_COLOR};

/**
 Generate a color string describing the color that would be used to display
 a particular piece of file metadata.

 @param Feature Pointer to the feature of metadata to obtain color information
        from.

 @param String On successful completion, updated to contain a string
        describing the color used to display the metadata feature.

 @param StringSize Specifies the size, in characters, of String.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirColorStringFromFeature(
    __in PCSDIR_FEATURE Feature,
    __out LPTSTR String,
    __in DWORD StringSize
    )
{
    WORD Forecolor;
    WORD Backcolor;

    LPTSTR Backstring;
    LPTSTR Forestring;

    if (Feature->Flags & SDIR_FEATURE_USE_FILE_COLOR) {
        YoriLibSPrintfS(String, StringSize, _T("file"));
        return TRUE;
    }

    Forecolor = (WORD)(Feature->HighlightColor.Win32Attr & SDIR_ATTRIBUTE_ONECOLOR_MASK);
    Backcolor = (WORD)((Feature->HighlightColor.Win32Attr >> 4) & SDIR_ATTRIBUTE_ONECOLOR_MASK);

    if (Feature->HighlightColor.Ctrl & SDIR_ATTRCTRL_WINDOW_BG) {
        Backstring = NULL;
    } else {
        Backstring = ColorString[Backcolor].String;
    }

    if (Feature->HighlightColor.Ctrl & SDIR_ATTRCTRL_WINDOW_FG) {
        Forestring = NULL;
    } else {
        Forestring = ColorString[Forecolor].String;
    }

    if (Backstring != NULL && Forestring != NULL) {
        YoriLibSPrintfS(String, StringSize, _T("bg_%s+%s"), Backstring, Forestring);
    } else if (Backstring != NULL) {
        YoriLibSPrintfS(String, StringSize, _T("bg_%s"), Backstring);
    } else if (Forestring != NULL) {
        YoriLibSPrintfS(String, StringSize, _T("%s"), Forestring);
    }

    return TRUE;
}

/**
 An in memory representation of a single match criteria, specifying the color
 to apply in event that the incoming file matches a specified criteria.
 */
typedef struct _SDIR_ATTRIBUTE_APPLY {

    /**
     Pointer to a function to compare an incoming directory entry against the
     dummy one contained here.
     */
    SDIR_COMPARE_FN CompareFn;

    /**
     An array indicating whether a match is found if the comparison returns
     less than, greater than, or equal.
     */
    BOOL TruthStates[3];

    /**
     The color to apply in event of a match.
     */
    YORILIB_COLOR_ATTRIBUTES Attribute;

    /**
     Unused data to explicitly preserve alignment.
     */
    WORD AlignmentPadding;

    /**
     A dummy directory entry containing values to compare against.  This is
     used to allow all compare functions to operate on two directory entries.
     */
    SDIR_DIRENT CompareEntry;
} SDIR_ATTRIBUTE_APPLY, *PSDIR_ATTRIBUTE_APPLY;


/**
 Populate a single SDIR_ATTRIBUTE_APPLY structure from a string indicating
 the operator, and a string indicating the criteria, and a feature index used
 to indicate which set of implementations should be used to apply the
 operator.

 @param ThisApply Pointer to the structure to populate.

 @param Operator Pointer to a NULL terminated string specifying the operator.

 @param Criteria Pointer to a NULL terminated string specifying the match
        criteria.

 @param FeatureNumber Specifies the index corresponding to the data source
        to compare against.  This is used to determine the appropriate
        comparison functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirPopulateApplyEntry(
    __in PSDIR_ATTRIBUTE_APPLY ThisApply,
    __in LPCTSTR Operator,
    __in LPCTSTR Criteria,
    __in DWORD FeatureNumber
    )
{
    PSDIR_FEATURE Feature;

    //
    //  Based on the operator, fill in the truth table.  We'll
    //  use the generic compare function and based on this
    //  truth table we'll decide whether the rule is satisfied
    //  or not.
    //

    if (Operator[0] == '>') {
        ThisApply->CompareFn = SdirOptions[FeatureNumber].CompareFn;

        ThisApply->TruthStates[SDIR_LESS_THAN] = FALSE;
        ThisApply->TruthStates[SDIR_GREATER_THAN] = TRUE;
        if (Operator[1] == '=') {
            ThisApply->TruthStates[SDIR_EQUAL] = TRUE;
        } else {
            ThisApply->TruthStates[SDIR_EQUAL] = FALSE;
        }
    } else if (Operator[0] == '<') {
        ThisApply->CompareFn = SdirOptions[FeatureNumber].CompareFn;

        ThisApply->TruthStates[SDIR_LESS_THAN] = TRUE;
        ThisApply->TruthStates[SDIR_GREATER_THAN] = FALSE;
        if (Operator[1] == '=') {
            ThisApply->TruthStates[SDIR_EQUAL] = TRUE;
        } else {
            ThisApply->TruthStates[SDIR_EQUAL] = FALSE;
        }
    } else if (Operator[0] == '!' && Operator[1] == '=') {
        ThisApply->CompareFn = SdirOptions[FeatureNumber].CompareFn;
        ThisApply->TruthStates[SDIR_LESS_THAN] = TRUE;
        ThisApply->TruthStates[SDIR_GREATER_THAN] = TRUE;
        ThisApply->TruthStates[SDIR_EQUAL] = FALSE;
    } else if (Operator[0] == '=') {
        ThisApply->CompareFn = SdirOptions[FeatureNumber].CompareFn;
        ThisApply->TruthStates[SDIR_LESS_THAN] = FALSE;
        ThisApply->TruthStates[SDIR_GREATER_THAN] = FALSE;
        ThisApply->TruthStates[SDIR_EQUAL] = TRUE;
    } else if (Operator[0] == '&') {
        ThisApply->CompareFn = SdirOptions[FeatureNumber].BitwiseCompareFn;
        ThisApply->TruthStates[SDIR_EQUAL] = TRUE;
        ThisApply->TruthStates[SDIR_NOT_EQUAL] = FALSE;
    } else if (Operator[0] == '!' && Operator[1] == '&') {
        ThisApply->CompareFn = SdirOptions[FeatureNumber].BitwiseCompareFn;
        ThisApply->TruthStates[SDIR_EQUAL] = FALSE;
        ThisApply->TruthStates[SDIR_NOT_EQUAL] = TRUE;
    } else {
        SdirWriteString(_T("Operator error in: "));
        return FALSE;
    }

    //
    //  Maybe the operator specified was valid but not supported
    //  by this type.
    //

    if (ThisApply->CompareFn == NULL) {
        SdirWriteString(_T("Operator not supported in: "));
        return FALSE;
    }

    //
    //  If we fail to capture this, ignore it and move on to the
    //  next entry, clobbering this structure again.
    //

    if (!SdirOptions[FeatureNumber].GenerateFromStringFn(&ThisApply->CompareEntry, Criteria)) {
        SdirWriteString(_T("Could not parse evalutation criteria in: "));
        return FALSE;
    }

    //
    //  Mark anything we're filtering by for collection
    //

    Feature = SdirFeatureByOptionNumber(FeatureNumber);
    Feature->Flags |= SDIR_FEATURE_COLLECT;
    return TRUE;
}

/**
 A global array of structures describing the criteria to use when applying
 colors to files.
 */
PSDIR_ATTRIBUTE_APPLY SdirAttributeApply = NULL; 

/**
 The default color attributes to apply for file criteria if the user has not
 specified anything else in the environment.
 */
const
CHAR SdirAttributeDefaultApplyString[] = 
    "fa&r,magenta;"
    "fa&D,lightmagenta;"
    "fa&R,green;"
    "fa&H,green;"
    "fa&S,green;"
    "fe=bat,lightred;"
    "fe=cmd,lightred;"
    "fe=com,lightcyan;"
    "fe=dll,cyan;"
    "fe=doc,white;"
    "fe=docx,white;"
    "fe=exe,lightcyan;"
    "fe=htm,white;"
    "fe=html,white;"
    "fe=pdf,white;"
    "fe=pl,red;"
    "fe=ppt,white;"
    "fe=pptx,white;"
    "fe=ps1,lightred;"
    "fe=psd1,red;"
    "fe=psm1,red;"
    "fe=sys,cyan;"
    "fe=xls,white;"
    "fe=xlsx,white;"
    "fe=ys1,lightred";

/**
 Parse the attribute strings from either the environment or the default set,
 and generate in memory SDIR_ATTRIBUTE_APPLY structures describing each
 criteria and resulting color to apply.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirParseAttributeApplyString()
{
    TCHAR SingleSwitch[20];
    LPTSTR Next;
    LPTSTR This;
    LPTSTR Operator;
    LPTSTR Criteria;
    LPTSTR Attribute;
    ULONG i;
    PSDIR_ATTRIBUTE_APPLY ThisApply;
    LPTSTR SdirAttributeCombinedApplyString = NULL;
    LPTSTR TokContext = NULL;
    DWORD  SdirColorPrependLength = 0;
    DWORD  SdirColorAppendLength  = 0;
    DWORD  SdirColorReplaceLength = 0;
    DWORD  SdirColorCustomLength = 0;

    //
    //  Load any user specified colors from the environment.  Prepend values go before
    //  any default; replace values supersede any default; and append values go last.
    //  We need to insert semicolons between these values if they're specified.
    //
    //  First, count how big the allocation needs to be and allocate it.
    //

    SdirColorPrependLength = GetEnvironmentVariable(_T("SDIR_COLOR_PREPEND"), NULL, 0);
    SdirColorReplaceLength = GetEnvironmentVariable(_T("SDIR_COLOR_REPLACE"), NULL, 0);
    SdirColorAppendLength  = GetEnvironmentVariable(_T("SDIR_COLOR_APPEND"), NULL, 0);
    if (Opts->CustomFileColor != NULL) {
        SdirColorCustomLength = Opts->CustomFileColorLength;
    }

    SdirAttributeCombinedApplyString = YoriLibMalloc((SdirColorPrependLength + 1 + SdirColorAppendLength + 1 + SdirColorReplaceLength + 1 + SdirColorCustomLength + sizeof(SdirAttributeDefaultApplyString)) * sizeof(TCHAR));

    if (SdirAttributeCombinedApplyString == NULL) {
        return FALSE;
    }

    //
    //  Now, load any environment variables into the buffer.  If replace isn't specified,
    //  we use the hardcoded default.
    //

    i = 0;
    if (SdirColorPrependLength) {
        GetEnvironmentVariable(_T("SDIR_COLOR_PREPEND"), SdirAttributeCombinedApplyString, SdirColorPrependLength);
        i += SdirColorPrependLength;
        SdirAttributeCombinedApplyString[i - 1] = ';';
    }

    if (SdirColorCustomLength) {
        YoriLibSPrintfS(SdirAttributeCombinedApplyString + i, SdirColorCustomLength + 1, _T("%s"), Opts->CustomFileColor);
        i += SdirColorCustomLength;
        SdirAttributeCombinedApplyString[i] = ';';
        i++;
    }

    if (SdirColorReplaceLength) {
        GetEnvironmentVariable(_T("SDIR_COLOR_REPLACE"), SdirAttributeCombinedApplyString + i, SdirColorReplaceLength);
        i += SdirColorReplaceLength - 1;
    } else {
        YoriLibSPrintfS(SdirAttributeCombinedApplyString + i, 
                    sizeof(SdirAttributeDefaultApplyString) / sizeof(SdirAttributeDefaultApplyString[0]),
                    _T("%hs"),
                    SdirAttributeDefaultApplyString);
        i += sizeof(SdirAttributeDefaultApplyString) / sizeof(SdirAttributeDefaultApplyString[0]) - 1;
    }

    if (SdirColorAppendLength) {
        SdirAttributeCombinedApplyString[i] = ';';
        i += 1;
        GetEnvironmentVariable(_T("SDIR_COLOR_APPEND"), SdirAttributeCombinedApplyString + i, SdirColorAppendLength);
    }

    //
    //  First count the number of rules so we can allocate the correct amount
    //  of memory for the set.  For ease of use, we allocate an additional one
    //  as a "null terminator."
    //

    This = SdirAttributeCombinedApplyString;
    i = 1;

    while (This != NULL) {

        i++;
        Next = _tcschr(This, ';');

        if (Next) {
            This = Next + 1;
        } else {
            This = NULL;
        }
    }

    if (Opts->CustomFileFilter != NULL) {
        This = Opts->CustomFileFilter;

        while (This != NULL) {

            i++;
            Next = _tcschr(This, ';');

            if (Next) {
                This = Next + 1;
            } else {
                This = NULL;
            }
        }
    }

    SdirAttributeApply = YoriLibMalloc(i * sizeof(SDIR_ATTRIBUTE_APPLY));
    if (SdirAttributeApply == NULL) {
        YoriLibFree(SdirAttributeCombinedApplyString);
        return FALSE;
    }

    ZeroMemory(SdirAttributeApply, i * sizeof(SDIR_ATTRIBUTE_APPLY));
    ThisApply = SdirAttributeApply;

    //
    //  First, process any filter/exclusion list, if present.
    //

    if (Opts->CustomFileFilter != NULL) {
        This = _tcstok_s(Opts->CustomFileFilter, _T(";"), &TokContext);

        //
        //  Now go through the rules again and populate the memory we just
        //  allocated with the comparison functions, criteria and attributes.
        //
    
        while (This != NULL) {
    
            //
            //  Ignore any leading spaces
            //
    
            while(*This == ' ') {
                This++;
            }
    
            //
            //  Count the number of chars to the operator.  Copy that
            //  many into the switch buffer, but not more than the
            //  size of the buffer.  We increment i to ensure we have
            //  space for the null terminator as well as the
            //  substring we want to process.
            //
    
            i = (ULONG)_tcscspn(This, _T("&<>=!"));
            if (i >= sizeof(SingleSwitch)/sizeof(SingleSwitch[0])) {
                i = sizeof(SingleSwitch)/sizeof(SingleSwitch[0]);
            } else {
                i++;
            }
    
            //
            //  Here we just give up on the 'secure' CRT things.  We
            //  want to copy the switch, up to the size of the buffer,
            //  and we're doing it manually to avoid puking because
            //  the source string may still (will still) have characters
            //  beyond this number.
            //
    
            memcpy(SingleSwitch, This, i * sizeof(TCHAR));
            SingleSwitch[i - 1] = '\0';
    
            Operator = This + i - 1;
    
            //
            //  Count the length of the operator and verify it's
            //  sane.
            //
    
            i = (ULONG)_tcsspn(Operator, _T("&<>=!"));
    
            if (i != 1 && i != 2) {
                SdirWriteString(_T("Operator error in: "));
                goto error_return;
            }
    
            //
            //  Count the length of the criteria and null terminate
            //  it for convenience.
            //
    
            Criteria = Operator + i;
    
            for (i = 0; i < SdirGetNumSdirOptions(); i++) {
                if (_tcsicmp(SingleSwitch, SdirOptions[i].Switch) == 0 &&
                    SdirOptions[i].GenerateFromStringFn != NULL) {
    
                    if (!SdirPopulateApplyEntry(ThisApply, Operator, Criteria, i)) {
                        goto error_return;
                    }
    
                    ThisApply->Attribute.Ctrl = SDIR_ATTRCTRL_HIDE;
                    ThisApply->Attribute.Win32Attr = 0;
    
                    ThisApply++;
                    break;
                }
            }
    
            if (i == SdirGetNumSdirOptions()) {
                SdirWriteString(_T("Criteria error in: "));
                goto error_return;
            }
    
            This = _tcstok_s(NULL, _T(";"), &TokContext);
        }

        //
        //  Since _tcstok just turned this into swiss cheese, deallocate it.
        //

        YoriLibFree(Opts->CustomFileFilter);
        Opts->CustomFileFilter = NULL;
    }

    This = _tcstok_s(SdirAttributeCombinedApplyString, _T(";"), &TokContext);

    //
    //  Now go through the rules again and populate the memory we just
    //  allocated with the comparison functions, criteria and attributes.
    //

    while (This != NULL) {

        //
        //  Ignore any leading spaces
        //

        while(*This == ' ') {
            This++;
        }

        //
        //  Count the number of chars to the operator.  Copy that
        //  many into the switch buffer, but not more than the
        //  size of the buffer.  We increment i to ensure we have
        //  space for the null terminator as well as the
        //  substring we want to process.
        //

        i = (ULONG)_tcscspn(This, _T("&<>=!"));
        if (i >= sizeof(SingleSwitch)/sizeof(SingleSwitch[0])) {
            i = sizeof(SingleSwitch)/sizeof(SingleSwitch[0]);
        } else {
            i++;
        }

        //
        //  Here we just give up on the 'secure' CRT things.  We
        //  want to copy the switch, up to the size of the buffer,
        //  and we're doing it manually to avoid puking because
        //  the source string may still (will still) have characters
        //  beyond this number.
        //

        memcpy(SingleSwitch, This, i * sizeof(TCHAR));
        SingleSwitch[i - 1] = '\0';

        Operator = This + i - 1;

        //
        //  Count the length of the operator and verify it's
        //  sane.
        //

        i = (ULONG)_tcsspn(Operator, _T("&<>=!"));

        if (i != 1 && i != 2) {
            SdirWriteString(_T("Operator error in: "));
            goto error_return;
        }

        //
        //  Count the length of the criteria and null terminate
        //  it for convenience.
        //

        Criteria = Operator + i;

        i = (ULONG)_tcscspn(Criteria, _T(","));
        if (Criteria[i] == '\0') {
            SdirWriteString(_T("Attribute error in: "));
            goto error_return;
        }

        Criteria[i] = '\0';
        Attribute = Criteria + i + 1;

        for (i = 0; i < SdirGetNumSdirOptions(); i++) {
            if (_tcsicmp(SingleSwitch, SdirOptions[i].Switch) == 0 &&
                SdirOptions[i].GenerateFromStringFn != NULL) {

                if (!SdirPopulateApplyEntry(ThisApply, Operator, Criteria, i)) {
                    goto error_return;
                }

                ThisApply->Attribute = YoriLibAttributeFromString(Attribute);
                ThisApply->Attribute.Ctrl &= ~(SDIR_ATTRCTRL_INVALID_FILE);

                ThisApply++;
                break;
            }
        }

        if (i == SdirGetNumSdirOptions()) {
            SdirWriteString(_T("Criteria error in: "));
            goto error_return;
        }

        This = _tcstok_s(NULL, _T(";"), &TokContext);
    }

    YoriLibFree(SdirAttributeCombinedApplyString);
    return TRUE;

error_return:

    SdirWriteString(This);
    SdirWriteString(_T("\n"));

    YoriLibFree(SdirAttributeCombinedApplyString);
    return FALSE;
}

/**
 Parse a string used to determine which color to use to display individual
 file metadata attributes.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirParseMetadataAttributeString()
{
    TCHAR SingleElement[256];
    TCHAR SingleSwitch[20];
    LPTSTR Next;
    LPTSTR This;
    LPTSTR Attribute;
    ULONG i;
    LPTSTR SdirApplyString = NULL;
    DWORD  SdirColorMetadataLength = 0;
    PSDIR_FEATURE Feature;

    //
    //  Load any user specified colors from the environment.
    //
    //  First, count how big the allocation needs to be and allocate it.
    //

    SdirColorMetadataLength = GetEnvironmentVariable(_T("SDIR_COLOR_METADATA"), NULL, 0);
    if (SdirColorMetadataLength == 0) {
        return TRUE;
    }
    SdirApplyString = YoriLibMalloc((SdirColorMetadataLength + 1) * sizeof(TCHAR));

    if (SdirApplyString == NULL) {
        return FALSE;
    }

    //
    //  Now, load any environment variables into the buffer.
    //

    GetEnvironmentVariable(_T("SDIR_COLOR_METADATA"), SdirApplyString, SdirColorMetadataLength);

    This = SdirApplyString;

    //
    //  Now go through the rules again and populate the memory we just
    //  allocated with the comparison functions, criteria and attributes.
    //

    while (This != NULL) {

        //
        //  We have a mutable buffer since we just allocated it.
        //  NULL-ing as we go is really a hint to new 'secure'
        //  versions of the CRT that we don't intend to copy more
        //  TCHARs than this, so it doesn't need to gracelessly
        //  puke.
        //

        Next = _tcschr(This, ';');
        if (Next) {
            *Next = '\0';
        }

        //
        //  Ignore any leading spaces
        //

        while(*This == ' ') {
            This++;
        }

        //
        //  We want to copy as many tchars as there are between two
        //  elements, but not more than the buffer size.  Note that
        //  the printf routine below will do the right thing at end
        //  of string so it's okay to pass a larger buffer size
        //  there.
        //

        i = sizeof(SingleElement)/sizeof(SingleElement[0]);

        if (Next && ((ULONG)(Next - This) < i)) {
            i = (ULONG)(Next - This + 1);
        }

        //
        //  If this element is empty, ignore it
        //

        if (i == 0 || *This == '\0') {
            goto next_element;
        }

        YoriLibSPrintfS(SingleElement, i, _T("%s"), This);
        SingleElement[i - 1] = '\0';

        //
        //  Count the number of chars to the operator.  Copy that
        //  many into the switch buffer, but not more than the
        //  size of the buffer.  We increment i to ensure we have
        //  space for the null terminator as well as the
        //  substring we want to process.
        //

        i = (ULONG)_tcscspn(SingleElement, _T(","));
        if (i >= sizeof(SingleSwitch)/sizeof(SingleSwitch[0])) {
            i = sizeof(SingleSwitch)/sizeof(SingleSwitch[0]);
        } else {
            i++;
        }

        //
        //  Here we just give up on the 'secure' CRT things.  We
        //  want to copy the switch, up to the size of the buffer,
        //  and we're doing it manually to avoid puking because
        //  the source string may still (will still) have characters
        //  beyond this number.
        //

        memcpy(SingleSwitch, SingleElement, i * sizeof(TCHAR));
        SingleSwitch[i - 1] = '\0';

        Attribute = SingleElement + i;

        for (i = 0; i < SdirGetNumSdirOptions(); i++) {
            if (_tcsicmp(SingleSwitch, SdirOptions[i].Switch) == 0) {

                YORILIB_COLOR_ATTRIBUTES HighlightColor;
                Feature = SdirFeatureByOptionNumber(i);

                if (Feature->Flags & SDIR_FEATURE_FIXED_COLOR) {
                    SdirWriteString(_T("Color cannot be changed for attribute in: "));
                    goto error_return;
                }

                //
                //  Set this piece of metadata to use the desired attributes.
                //  If it's invalid, just ignore it.  Hack around formats so
                //  we can remember to ignore this color and use the file's
                //  color in the flags field and retain color in the attribute.
                //

                HighlightColor = YoriLibAttributeFromString(Attribute);
                if (HighlightColor.Ctrl & SDIR_ATTRCTRL_INVALID_METADATA) {
                    SdirWriteString(_T("Invalid color specified in: "));
                    goto error_return;
                    break;
                }

                Feature->HighlightColor = YoriLibResolveWindowColorComponents(HighlightColor, Opts->PreviousAttributes, FALSE);

                if (HighlightColor.Ctrl & SDIR_ATTRCTRL_FILE) {
                    Feature->Flags |= SDIR_FEATURE_USE_FILE_COLOR;
                } else {
                    Feature->Flags &= ~(SDIR_FEATURE_USE_FILE_COLOR);
                }
                break;
            }
        }

next_element:

        if (Next) {
            This = Next + 1;
        } else {
            This = NULL;
        }
    }

    YoriLibFree(SdirApplyString);
    return TRUE;

error_return:

    SdirWriteString(SingleElement);
    SdirWriteString(_T("\n"));

    YoriLibFree(SdirApplyString);
    return FALSE;
}

/**
 Apply the previously loaded set of attributes to apply in response to
 criteria against a user file.

 @param DirEnt Pointer to the found file, with all relevant information loaded.

 @param Attribute On successful completion, indicates the color to use to
        display the file.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirApplyAttribute(
    __in PSDIR_DIRENT DirEnt,
    __out PYORILIB_COLOR_ATTRIBUTES Attribute
    )
{
    PSDIR_ATTRIBUTE_APPLY ThisApply;
    YORILIB_COLOR_ATTRIBUTES ThisAttribute = {SDIR_ATTRCTRL_WINDOW_BG | SDIR_ATTRCTRL_WINDOW_FG, 0};

    if (SdirAttributeApply == NULL) {
        return FALSE;
    }
    
    ThisApply = SdirAttributeApply;

    while (ThisApply->CompareFn) {

        if (ThisApply->TruthStates[ThisApply->CompareFn(DirEnt, &ThisApply->CompareEntry)]) {
            ThisAttribute = YoriLibCombineColors(ThisAttribute, ThisApply->Attribute);
            if ((ThisAttribute.Ctrl & SDIR_ATTRCTRL_CONTINUE) == 0) {

                ThisAttribute = YoriLibResolveWindowColorComponents(ThisAttribute, Opts->PreviousAttributes, FALSE);

                if (ThisAttribute.Ctrl & SDIR_ATTRCTRL_INVERT) {
                    ThisAttribute.Win32Attr = (UCHAR)(((ThisAttribute.Win32Attr & 0x0F) << 4) + ((ThisAttribute.Win32Attr & 0xF0) >> 4));
                }

                *Attribute = ThisAttribute;
                return TRUE;
            }

            ThisAttribute.Ctrl = (UCHAR)(ThisAttribute.Ctrl & ~(SDIR_ATTRCTRL_CONTINUE));
        }

        ThisApply++;
    }

    //
    //  We do let the user explicitly request black on black, but
    //  if we ended the search due to unbounded continues, return
    //  what we have.
    //

    if (ThisAttribute.Ctrl & SDIR_ATTRCTRL_TERMINATE_MASK || ThisAttribute.Win32Attr != 0) {

        *Attribute = ThisAttribute;
        return TRUE;
    }

    *Attribute = SdirDefaultColor;
    
    return FALSE;
}

// vim:sw=4:ts=4:et:
