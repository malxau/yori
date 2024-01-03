/**
 * @file sdir/color.c
 *
 * Colorful, sorted and optionally rich directory enumeration
 * for Windows.
 *
 * This module implements string parsing and rule application to select a
 * given set of color attributes to render any particular file with.
 *
 * Copyright (c) 2014-2019 Malcolm J. Smith
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
YORILIB_COLOR_ATTRIBUTES SdirDefaultColor = {YORILIB_ATTRCTRL_WINDOW_BG | YORILIB_ATTRCTRL_WINDOW_FG, SDIR_DEFAULT_COLOR};

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
    __in YORI_ALLOC_SIZE_T StringSize
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

    Forecolor = (WORD)(Feature->HighlightColor.Win32Attr & YORILIB_ATTRIBUTE_ONECOLOR_MASK);
    Backcolor = (WORD)((Feature->HighlightColor.Win32Attr >> 4) & YORILIB_ATTRIBUTE_ONECOLOR_MASK);

    if (Feature->HighlightColor.Ctrl & YORILIB_ATTRCTRL_WINDOW_BG) {
        Backstring = NULL;
    } else {
        Backstring = YoriLibColorStringTable[Backcolor].String;
    }

    if (Feature->HighlightColor.Ctrl & YORILIB_ATTRCTRL_WINDOW_FG) {
        Forestring = NULL;
    } else {
        Forestring = YoriLibColorStringTable[Forecolor].String;
    }

    if (Backstring != NULL && Forestring != NULL) {
        YoriLibSPrintfS(String, StringSize, _T("bg_%s+%s"), Backstring, Forestring);
    } else if (Backstring != NULL) {
        YoriLibSPrintfS(String, StringSize, _T("bg_%s"), Backstring);
    } else if (Forestring != NULL) {
        YoriLibSPrintfS(String, StringSize, _T("%s"), Forestring);
    } else {
        YoriLibSPrintfS(String, StringSize, _T(""));
    }

    return TRUE;
}

/**
 Go through each matching criteria in a filter, and ensure the corresponding
 feature is marked for collection.  This function assumes that the function
 pointers between the two must be the same.

 @param Filter The list of criteria to collect.

 @param SanitizeColor If TRUE, any color specified should be sanitized,
        removing meaningless values.  Note this implies that each match
        is for a color match.  If FALSE, no assumption is made about the
        type of the match.
 */
VOID
SdirMarkFeaturesForCollection(
    __in PYORI_LIB_FILE_FILTER Filter,
    __in BOOL SanitizeColor
    )
{
    PYORI_LIB_FILE_FILT_MATCH_CRITERIA ThisMatch;
    DWORD Index;
    DWORD FeatureNumber;
    PSDIR_FEATURE Feature;

    for (Index = 0; Index < Filter->NumberCriteria; Index++) {
        ThisMatch = (PYORI_LIB_FILE_FILT_MATCH_CRITERIA)YoriLibAddToPointer(Filter->Criteria, Index * Filter->ElementSize);
        if (ThisMatch->CollectFn != NULL) {
            for (FeatureNumber = 0; FeatureNumber < SdirGetNumSdirOptions(); FeatureNumber++) {
                if (ThisMatch->CollectFn == SdirOptions[FeatureNumber].CollectFn) {

                    Feature = SdirFeatureByOptionNumber(FeatureNumber);
                    Feature->Flags |= SDIR_FEATURE_COLLECT;
                    break;
                }
            }
            ASSERT(FeatureNumber < SdirGetNumSdirOptions());
        }
        if (SanitizeColor) {
            PYORI_LIB_FILE_FILT_COLOR_CRITERIA ThisColor;
            ThisColor = (PYORI_LIB_FILE_FILT_COLOR_CRITERIA)ThisMatch;
            ThisColor->Color.Ctrl &= ~(SDIR_ATTRCTRL_INVALID_FILE);
        }
    }
}

/**
 Parse the attribute strings from either the environment or the default set,
 and generate in memory SDIR_ATTRIBUTE_APPLY structures describing each
 criteria and resulting color to apply.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirParseAttributeApplyString(VOID)
{
    LPTSTR This;
    YORI_STRING Combined;
    YORI_STRING ErrorSubstring;

    if (!YoriLibLoadCombinedFileColorString(&Opts->CustomFileColor, &Combined)) {
        return FALSE;
    }

    //
    //  Now that we have a single string from all sources, parse the string
    //  into a series of actions to apply.  Start with the objects to hide.
    //

    if (Opts->CustomFileFilter.LengthInChars > 0) {
        if (!YoriLibFileFiltParseFilterString(&SdirGlobal.FileHideCriteria, &Opts->CustomFileFilter, &ErrorSubstring)) {
            This = YoriLibCStringFromYoriString(&ErrorSubstring);
            goto error_substring_return;
        }
        SdirMarkFeaturesForCollection(&SdirGlobal.FileHideCriteria, FALSE);
    }

    //
    //  Now look for colors to apply in response to specific criteria.
    //

    if (!YoriLibFileFiltParseColorString(&SdirGlobal.FileColorCriteria, &Combined, &ErrorSubstring)) {
        This = YoriLibCStringFromYoriString(&ErrorSubstring);
        goto error_substring_return;
    }
    SdirMarkFeaturesForCollection(&SdirGlobal.FileColorCriteria, TRUE);

    YoriLibFreeStringContents(&Combined);
    return TRUE;

error_substring_return:

    if (This != NULL) {
        SdirWriteString(_T("Parse error in: "));
        SdirWriteString(This);
        YoriLibDereference(This);
    }

    SdirWriteString(_T("\n"));
    YoriLibFreeStringContents(&Combined);

    return FALSE;
}

/**
 Parse a string used to determine which color to use to display individual
 file metadata attributes.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirParseMetadataAttributeString(VOID)
{
    TCHAR SingleElement[256];
    TCHAR SingleSwitch[20];
    LPTSTR Next;
    LPTSTR This;
    LPTSTR Attribute;
    YORI_ALLOC_SIZE_T i;
    LPTSTR SdirApplyString = NULL;
    YORI_ALLOC_SIZE_T SdirColorMetadataLength = 0;
    PSDIR_FEATURE Feature;
    LPTSTR EnvVarName = _T("YORICOLORMETADATA");

    //
    //  Load any user specified colors from the environment.
    //
    //  First, count how big the allocation needs to be and allocate it.
    //

    SdirColorMetadataLength = (YORI_ALLOC_SIZE_T)GetEnvironmentVariable(EnvVarName, NULL, 0);
    if (SdirColorMetadataLength == 0) {
        EnvVarName = _T("SDIR_COLOR_METADATA");
        SdirColorMetadataLength = (YORI_ALLOC_SIZE_T)GetEnvironmentVariable(EnvVarName, NULL, 0);
        if (SdirColorMetadataLength == 0) {
            return TRUE;
        }
    }
    SdirApplyString = YoriLibMalloc((SdirColorMetadataLength + 1) * sizeof(TCHAR));

    if (SdirApplyString == NULL) {
        return FALSE;
    }

    //
    //  Now, load any environment variables into the buffer.
    //

    GetEnvironmentVariable(EnvVarName, SdirApplyString, SdirColorMetadataLength);

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

        if (Next && ((YORI_ALLOC_SIZE_T)(Next - This) < i)) {
            i = (YORI_ALLOC_SIZE_T)(Next - This + 1);
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

        i = (YORI_ALLOC_SIZE_T)_tcscspn(SingleElement, _T(","));
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

                YoriLibAttributeFromLiteralString(Attribute, &HighlightColor);
                if (HighlightColor.Ctrl & SDIR_ATTRCTRL_INVALID_METADATA) {
                    SdirWriteString(_T("Invalid color specified in: "));
                    goto error_return;
                    break;
                }

                YoriLibResolveWindowColorComponents(HighlightColor, Opts->PreviousAttributes, FALSE, &Feature->HighlightColor);

                if (HighlightColor.Ctrl & YORILIB_ATTRCTRL_FILE) {
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

 @param ForceDisplay If TRUE, the entry needs to be displayed and all rules
        indicating that it should be hidden are ignored.  This is used to
        ensure things like directory headings are not displayed as hidden.

 @param Attribute On successful completion, indicates the color to use to
        display the file.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirApplyAttribute(
    __in PYORI_FILE_INFO DirEnt,
    __in BOOL ForceDisplay,
    __out PYORILIB_COLOR_ATTRIBUTES Attribute
    )
{
    DWORD Index;
    YORILIB_COLOR_ATTRIBUTES ThisAttribute;

    ThisAttribute.Ctrl = YORILIB_ATTRCTRL_WINDOW_BG | YORILIB_ATTRCTRL_WINDOW_FG;
    ThisAttribute.Win32Attr = 0;

    //
    //  First check for files to hide.
    //

    if (!ForceDisplay) {

        PYORI_LIB_FILE_FILT_MATCH_CRITERIA ThisMatch;
        PYORI_LIB_FILE_FILT_MATCH_CRITERIA Matches;

        //
        //  We expect each element to just be the criteria determining a
        //  match, since the action to take is already known.
        //

        ASSERT((SdirGlobal.FileHideCriteria.ElementSize == 0 &&
                SdirGlobal.FileHideCriteria.NumberCriteria == 0) ||
               SdirGlobal.FileHideCriteria.ElementSize == sizeof(YORI_LIB_FILE_FILT_MATCH_CRITERIA));

        Matches = (PYORI_LIB_FILE_FILT_MATCH_CRITERIA)SdirGlobal.FileHideCriteria.Criteria;
        for (Index = 0; Index < SdirGlobal.FileHideCriteria.NumberCriteria; Index++) {
            ThisMatch = &Matches[Index];
            if (ThisMatch->TruthStates[ThisMatch->CompareFn(DirEnt, &ThisMatch->CompareEntry)]) {
                ThisAttribute.Ctrl |= YORILIB_ATTRCTRL_HIDE;
                Attribute->Ctrl = ThisAttribute.Ctrl;
                Attribute->Win32Attr = ThisAttribute.Win32Attr;
                return TRUE;
            }
        }
    }

    //
    //  Now check for the color to apply to files which are not hidden.
    //

    {
        PYORI_LIB_FILE_FILT_COLOR_CRITERIA ThisApply;
        PYORI_LIB_FILE_FILT_COLOR_CRITERIA ColorsToApply;

        //
        //  We expect each element to be the criteria determining a match and
        //  color to apply in event of a match.
        //

        ASSERT((SdirGlobal.FileColorCriteria.ElementSize == 0 &&
                SdirGlobal.FileColorCriteria.NumberCriteria == 0) ||
               SdirGlobal.FileColorCriteria.ElementSize == sizeof(YORI_LIB_FILE_FILT_COLOR_CRITERIA));
        ColorsToApply = (PYORI_LIB_FILE_FILT_COLOR_CRITERIA)SdirGlobal.FileColorCriteria.Criteria;
        for (Index = 0; Index < SdirGlobal.FileColorCriteria.NumberCriteria; Index++) {
            ThisApply = &ColorsToApply[Index];
            if (ThisApply->Match.TruthStates[ThisApply->Match.CompareFn(DirEnt, &ThisApply->Match.CompareEntry)]) {
                YoriLibCombineColors(ThisAttribute, ThisApply->Color, &ThisAttribute);
                if ((!ForceDisplay || (ThisAttribute.Ctrl & YORILIB_ATTRCTRL_HIDE) == 0) &&
                    (ThisAttribute.Ctrl & YORILIB_ATTRCTRL_CONTINUE) == 0) {

                    YoriLibResolveWindowColorComponents(ThisAttribute, Opts->PreviousAttributes, TRUE, &ThisAttribute);

                    if (ThisAttribute.Ctrl & YORILIB_ATTRCTRL_INVERT) {
                        ThisAttribute.Win32Attr = (UCHAR)(((ThisAttribute.Win32Attr & 0x0F) << 4) + ((ThisAttribute.Win32Attr & 0xF0) >> 4));
                    }

                    Attribute->Ctrl = ThisAttribute.Ctrl;
                    Attribute->Win32Attr = ThisAttribute.Win32Attr;
                    return TRUE;
                }

                ThisAttribute.Ctrl = (UCHAR)(ThisAttribute.Ctrl & ~(YORILIB_ATTRCTRL_CONTINUE));
            }
        }
    }

    //
    //  We do let the user explicitly request black on black, but
    //  if we ended the search due to unbounded continues, return
    //  what we have.
    //

    if (ThisAttribute.Ctrl & YORILIB_ATTRCTRL_TERMINATE_MASK || ThisAttribute.Win32Attr != 0) {

        Attribute->Ctrl = ThisAttribute.Ctrl;
        Attribute->Win32Attr = ThisAttribute.Win32Attr;
        return TRUE;
    }

    Attribute->Ctrl = SdirDefaultColor.Ctrl;
    Attribute->Win32Attr = SdirDefaultColor.Win32Attr;

    return FALSE;
}

// vim:sw=4:ts=4:et:
