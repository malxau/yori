/**
 * @file lib/filefilt.c
 *
 * Yori shell filter enumerated files according to criteria
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
 * FITNESS YORI_LIB_FILE_FILT A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE YORI_LIB_FILE_FILT ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <yoripch.h>
#include <yorilib.h>
#ifdef YORI_BUILTIN
#include <yoricall.h>
#endif

/**
 Help text to display to the user.
 */
const
CHAR strFileFiltHelpText[] =
        "\n"
        " Valid operators are:\n"
        "   =   File attribute matches criteria\n"
        "   !=  File attribute does not match criteria\n"
        "   >   File attribute greater than criteria\n"
        "   >=  File attribute greater than or equal to criteria\n"
        "   <   File attribute less than criteria\n"
        "   <=  File attribute less than or equal to criteria\n"
        "   &   File attribute includes criteria or wildcard string\n"
        "   !&  File attribute does not include criteria or wildcard string\n"
        "\n"
        " Valid attributes are:\n";

/**
 A single option that files can be filtered against.
 */
typedef struct _YORI_LIB_FILE_FILT_FILTER_OPT {

    /**
     The two character switch name for the option and a NULL terminator.
     */
    TCHAR Switch[3];

    /**
     Pointer to a function to collect the data from a specific file for the
     option.
     */
    YORI_LIB_FILE_FILT_COLLECT_FN CollectFn;

    /**
     Pointer to a function to compare the user supplied value against the
     value from a given file.
     */
    YORI_LIB_FILE_FILT_COMPARE_FN CompareFn;

    /**
     Pointer to a function to compare the user supplied value against the
     value from a given file in a bitwise fashion.
     */
    YORI_LIB_FILE_FILT_COMPARE_FN BitwiseCompareFn;

    /**
     Pointer to a function to convert the user supplied string into a value
     for the option.
     */
    YORI_LIB_FILE_FILT_GENERATE_FROM_STRING_FN GenerateFromStringFn;

    /**
     A string containing a description for the option.
     */
    CHAR Help[24];
} YORI_LIB_FILE_FILT_FILTER_OPT, *PYORI_LIB_FILE_FILT_FILTER_OPT;

/**
 A pointer to a filter option that cannot change.  Because options are
 compiled into the binary in read only form, all pointers should really
 be const pointers.
 */
typedef YORI_LIB_FILE_FILT_FILTER_OPT CONST * PCYORI_LIB_FILE_FILT_FILTER_OPT;

/**
 An array of options that are supported by this program.
 */
CONST YORI_LIB_FILE_FILT_FILTER_OPT
YoriLibFileFiltFilterOptions[] = {
    {_T("ac"),                               YoriLibCollectAllocatedRangeCount,
     YoriLibCompareAllocatedRangeCount,      NULL,
     YoriLibGenerateAllocatedRangeCount,     "allocated range count"},

    {_T("ad"),                               YoriLibCollectAccessTime,
     YoriLibCompareAccessDate,               NULL,
     YoriLibGenerateAccessDate,              "access date"},

    {_T("ar"),                               YoriLibCollectArch,
     YoriLibCompareArch,                     NULL,
     YoriLibGenerateArch,                    "CPU architecture"},

    {_T("as"),                               YoriLibCollectAllocationSize,
     YoriLibCompareAllocationSize,           NULL,
     YoriLibGenerateAllocationSize,          "allocation size"},

    {_T("at"),                               YoriLibCollectAccessTime,
     YoriLibCompareAccessTime,               NULL,
     YoriLibGenerateAccessTime,              "access time"},

    {_T("ca"),                               YoriLibCollectCompressionAlgorithm,
     YoriLibCompareCompressionAlgorithm,     NULL,
     YoriLibGenerateCompressionAlgorithm,    "compression algorithm"},

    {_T("cd"),                               YoriLibCollectCreateTime,
     YoriLibCompareCreateDate,               NULL,
     YoriLibGenerateCreateDate,              "create date"},

    {_T("cs"),                               YoriLibCollectCompressedFileSize,
     YoriLibCompareCompressedFileSize,       NULL,
     YoriLibGenerateCompressedFileSize,      "compressed size"},

    {_T("ct"),                               YoriLibCollectCreateTime,
     YoriLibCompareCreateTime,               NULL,
     YoriLibGenerateCreateTime,              "create time"},

    {_T("de"),                               YoriLibCollectDescription,
     YoriLibCompareDescription,              NULL,
     YoriLibGenerateDescription,             "description"},

    {_T("ep"),                               YoriLibCollectEffectivePermissions,
     YoriLibCompareEffectivePermissions,     YoriLibBitwiseEffectivePermissions,
     YoriLibGenerateEffectivePermissions,    "effective permissions"},

    {_T("fa"),                               YoriLibCollectFileAttributes,
     YoriLibCompareFileAttributes,           YoriLibBitwiseFileAttributes,
     YoriLibGenerateFileAttributes,          "file attributes"},

    {_T("fc"),                               YoriLibCollectFragmentCount,
     YoriLibCompareFragmentCount,            NULL,
     YoriLibGenerateFragmentCount,           "fragment count"},

    {_T("fe"),                               YoriLibCollectFileName,
     YoriLibCompareFileExtension,            NULL,
     YoriLibGenerateFileExtension,           "file extension"},

    {_T("fi"),                               YoriLibCollectFileId,
     YoriLibCompareFileId,                   NULL,
     YoriLibGenerateFileId,                  "file id"},

    {_T("fn"),                               YoriLibCollectFileName,
     YoriLibCompareFileName,                 YoriLibBitwiseFileName,
     YoriLibGenerateFileName,                "file name"},

    {_T("fs"),                               YoriLibCollectFileSize,
     YoriLibCompareFileSize,                 NULL,
     YoriLibGenerateFileSize,                "file size"},

    {_T("fv"),                               YoriLibCollectFileVersionString,
     YoriLibCompareFileVersionString,        NULL,
     YoriLibGenerateFileVersionString,       "file version string"},

    {_T("lc"),                               YoriLibCollectLinkCount,
     YoriLibCompareLinkCount,                NULL,
     YoriLibGenerateLinkCount,               "link count"},

    {_T("oi"),                               YoriLibCollectObjectId,
     YoriLibCompareObjectId,                 NULL,
     YoriLibGenerateObjectId,                "object id"},

    {_T("os"),                               YoriLibCollectOsVersion,
     YoriLibCompareOsVersion,                NULL,
     YoriLibGenerateOsVersion,               "minimum OS version"},

    {_T("ow"),                               YoriLibCollectOwner,
     YoriLibCompareOwner,                    NULL,
     YoriLibGenerateOwner,                   "owner"},

    {_T("rt"),                               YoriLibCollectReparseTag,
     YoriLibCompareReparseTag,               NULL,
     YoriLibGenerateReparseTag,              "reparse tag"},

    {_T("sc"),                               YoriLibCollectStreamCount,
     YoriLibCompareStreamCount,              NULL,
     YoriLibGenerateStreamCount,             "stream count"},

    {_T("sn"),                               YoriLibCollectShortName,
     YoriLibCompareShortName,                NULL,
     YoriLibGenerateShortName,               "short name"},

    {_T("ss"),                               YoriLibCollectSubsystem,
     YoriLibCompareSubsystem,                NULL,
     YoriLibGenerateSubsystem,               "subsystem"},

    {_T("us"),                               YoriLibCollectUsn,
     YoriLibCompareUsn,                      NULL,
     YoriLibGenerateUsn,                     "USN"},

    {_T("vr"),                               YoriLibCollectVersion,
     YoriLibCompareVersion,                  NULL,
     YoriLibGenerateVersion,                 "version"},

    {_T("wd"),                               YoriLibCollectWriteTime,
     YoriLibCompareWriteDate,                NULL,
     YoriLibGenerateWriteDate,               "write date"},

    {_T("wt"),                               YoriLibCollectWriteTime,
     YoriLibCompareWriteTime,                NULL,
     YoriLibGenerateWriteTime,               "write time"},
};

/**
 Display usage text to the user.
 */
BOOL
YoriLibFileFiltHelp(VOID)
{
    DWORD Index;

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strFileFiltHelpText);

    //
    //  Display supported options and operators
    //

    for (Index = 0; Index < sizeof(YoriLibFileFiltFilterOptions)/sizeof(YoriLibFileFiltFilterOptions[0]); Index++) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, 
                      _T("   %s (%hs), %hs%hs%hs\n"),
                      YoriLibFileFiltFilterOptions[Index].Switch,
                      YoriLibFileFiltFilterOptions[Index].Help,
                      YoriLibFileFiltFilterOptions[Index].CompareFn?"=, !=, >, >=, <, <=":"",
                      (YoriLibFileFiltFilterOptions[Index].CompareFn && YoriLibFileFiltFilterOptions[Index].BitwiseCompareFn)?", ":"",
                      YoriLibFileFiltFilterOptions[Index].BitwiseCompareFn?"&, !&":"");

    }
    return TRUE;
}


/**
 Parse a user supplied operator and user supplied value for an option that
 has already been found.  This function needs to validate whether the
 operator and value make sense, and if so, populate the Criteria option with
 function pointers, a truth table, and the user's value string resolved into
 a machine value to compare against.

 @param Criteria Pointer to the criteria object to populate with the result
        of this function.

 @param Operator Pointer to the string describing the operator.  Note this
        string is not NULL terminated since it refers to a portion of the
        user's string.

 @param Value Pointer to the string to compare against.  Note this string is
        not NULL terminated since it refers to a portion of the user's string.

 @param MatchedOption Pointer to the option that has matched against the
        option that the user is trying to impose.  This provides the source
        for how to implement the operators and how to resolve the value.

 @param ErrorSubstring On failure, updated to point to the part of the user's
        expression that caused the failure.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibFileFiltParseFilterOperator(
    __out PYORI_LIB_FILE_FILT_MATCH_CRITERIA Criteria,
    __in PYORI_STRING Operator,
    __in PYORI_STRING Value,
    __in PCYORI_LIB_FILE_FILT_FILTER_OPT MatchedOption,
    __out _On_failure_(_Post_valid_) PYORI_STRING ErrorSubstring
    )
{
    //
    //  Based on the operator, fill in the truth table.  We'll
    //  use the generic compare function and based on this
    //  truth table we'll decide whether the rule is satisfied
    //  or not.
    //

    YoriLibInitEmptyString(ErrorSubstring);
    if (YoriLibCompareStringWithLiteral(Operator, _T(">")) == 0) {
        Criteria->CompareFn = MatchedOption->CompareFn;
        Criteria->TruthStates[YORI_LIB_LESS_THAN] = FALSE;
        Criteria->TruthStates[YORI_LIB_GREATER_THAN] = TRUE;
        Criteria->TruthStates[YORI_LIB_EQUAL] = FALSE;
    } else if (YoriLibCompareStringWithLiteral(Operator, _T(">=")) == 0) {
        Criteria->CompareFn = MatchedOption->CompareFn;
        Criteria->TruthStates[YORI_LIB_LESS_THAN] = FALSE;
        Criteria->TruthStates[YORI_LIB_GREATER_THAN] = TRUE;
        Criteria->TruthStates[YORI_LIB_EQUAL] = TRUE;
    } else if (YoriLibCompareStringWithLiteral(Operator, _T("<")) == 0) {
        Criteria->CompareFn = MatchedOption->CompareFn;
        Criteria->TruthStates[YORI_LIB_LESS_THAN] = TRUE;
        Criteria->TruthStates[YORI_LIB_GREATER_THAN] = FALSE;
        Criteria->TruthStates[YORI_LIB_EQUAL] = FALSE;
    } else if (YoriLibCompareStringWithLiteral(Operator, _T("<=")) == 0) {
        Criteria->CompareFn = MatchedOption->CompareFn;
        Criteria->TruthStates[YORI_LIB_LESS_THAN] = TRUE;
        Criteria->TruthStates[YORI_LIB_GREATER_THAN] = FALSE;
        Criteria->TruthStates[YORI_LIB_EQUAL] = TRUE;
    } else if (YoriLibCompareStringWithLiteral(Operator, _T("!=")) == 0) {
        Criteria->CompareFn = MatchedOption->CompareFn;
        Criteria->TruthStates[YORI_LIB_LESS_THAN] = TRUE;
        Criteria->TruthStates[YORI_LIB_GREATER_THAN] = TRUE;
        Criteria->TruthStates[YORI_LIB_EQUAL] = FALSE;
    } else if (YoriLibCompareStringWithLiteral(Operator, _T("=")) == 0) {
        Criteria->CompareFn = MatchedOption->CompareFn;
        Criteria->TruthStates[YORI_LIB_LESS_THAN] = FALSE;
        Criteria->TruthStates[YORI_LIB_GREATER_THAN] = FALSE;
        Criteria->TruthStates[YORI_LIB_EQUAL] = TRUE;
    } else if (YoriLibCompareStringWithLiteral(Operator, _T("&")) == 0) {
        Criteria->CompareFn = MatchedOption->BitwiseCompareFn;
        Criteria->TruthStates[YORI_LIB_EQUAL] = TRUE;
        Criteria->TruthStates[YORI_LIB_NOT_EQUAL] = FALSE;
    } else if (YoriLibCompareStringWithLiteral(Operator, _T("!&")) == 0) {
        Criteria->CompareFn = MatchedOption->BitwiseCompareFn;
        Criteria->TruthStates[YORI_LIB_EQUAL] = FALSE;
        Criteria->TruthStates[YORI_LIB_NOT_EQUAL] = TRUE;
    } else {
        ErrorSubstring->StartOfString = Operator->StartOfString;
        ErrorSubstring->LengthInChars = Operator->LengthInChars;
        return FALSE;
    }

    //
    //  Maybe the operator specified was valid but not supported
    //  by this type.
    //

    if (Criteria->CompareFn == NULL) {
        ErrorSubstring->StartOfString = Operator->StartOfString;
        ErrorSubstring->LengthInChars = Operator->LengthInChars;
        return FALSE;
    }

    Criteria->CollectFn = MatchedOption->CollectFn;

    //
    //  If we fail to capture this, ignore it and move on to the
    //  next entry, clobbering this structure again.
    //

    if (!MatchedOption->GenerateFromStringFn(&Criteria->CompareEntry, Value)) {
        ErrorSubstring->StartOfString = Value->StartOfString;
        ErrorSubstring->LengthInChars = Value->LengthInChars;
        return FALSE;
    }

    return TRUE;
}

/**
 Parse a single user supplied option string that commences with an option
 followed by an operator.  This function resolves the option into a pointer
 to its structure, and validates the operator and returns a substring to it.

 @param FilterElement Pointer to the string describing a single thing to
        filter against.  This string should indicate the option and operator.
        This function needs to perform validation that these details are
        provided and are meaningful.  Note this string is not NULL terminated
        since it refers to a portion of the user's string.

 @param Opt On successful completion, updated to point to the in memory object
        describing the user specified option.

 @param Operator On successful completion, updated to point to the substring
        within FilterElement that refers to the operator.

 @param ErrorSubstring On failure, updated to point to the part of the user's
        expression that caused the failure.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibFileFiltParseFilterOptAndOperator(
    __in PYORI_STRING FilterElement,
    __out PCYORI_LIB_FILE_FILT_FILTER_OPT * Opt,
    __out PYORI_STRING Operator,
    __out _On_failure_(_Post_valid_) PYORI_STRING ErrorSubstring
    )
{
    YORI_STRING SwitchName;
    DWORD Count;
    PCYORI_LIB_FILE_FILT_FILTER_OPT FoundOpt;

    YoriLibInitEmptyString(&SwitchName);
    YoriLibInitEmptyString(ErrorSubstring);

    SwitchName.StartOfString = FilterElement->StartOfString;
    SwitchName.LengthInChars = FilterElement->LengthInChars;

    YoriLibTrimSpaces(&SwitchName);

    SwitchName.LengthInChars = YoriLibCountStringNotContainingChars(&SwitchName, _T("&<>=!"));
    FoundOpt = NULL;

    for (Count = 0; Count < sizeof(YoriLibFileFiltFilterOptions)/sizeof(YoriLibFileFiltFilterOptions[0]); Count++) {
        if (YoriLibCompareStringWithLiteralInsensitive(&SwitchName, YoriLibFileFiltFilterOptions[Count].Switch) == 0) {
            FoundOpt = &YoriLibFileFiltFilterOptions[Count];
            break;
        }
    }

    if (FoundOpt == NULL) {
        ErrorSubstring->StartOfString = SwitchName.StartOfString;
        ErrorSubstring->LengthInChars = SwitchName.LengthInChars;
        return FALSE;
    }

    *Opt = FoundOpt;

    YoriLibInitEmptyString(Operator);

    Operator->StartOfString = SwitchName.StartOfString + SwitchName.LengthInChars;
    Operator->LengthInChars = FilterElement->LengthInChars - SwitchName.LengthInChars - (DWORD)(SwitchName.StartOfString - FilterElement->StartOfString);

    Operator->LengthInChars = YoriLibCountStringContainingChars(Operator, _T("&<>=!"));
    return TRUE;
}

/**
 Parse a single user supplied option string into a criteria to apply against
 each file found.  This function performs all string validation.

 @param Criteria Pointer to the criteria object to populate with the result
        of this function.

 @param FilterElement Pointer to the string describing a single thing to
        filter against.  This string should indicate the option, operator,
        and value.  This function needs to perform all validation that these
        details are provided and are meaningful.  Note this string is not
        NULL terminated since it refers to a portion of the user's string.

 @param ErrorSubstring On failure, updated to point to the part of the user's
        expression that caused the failure.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibFileFiltParseFilterElement(
    __out PYORI_LIB_FILE_FILT_MATCH_CRITERIA Criteria,
    __in PYORI_STRING FilterElement,
    __out _On_failure_(_Post_valid_) PYORI_STRING ErrorSubstring
    )
{
    YORI_STRING Operator;
    YORI_STRING Value;
    PCYORI_LIB_FILE_FILT_FILTER_OPT FoundOpt;

    if (!YoriLibFileFiltParseFilterOptAndOperator(FilterElement, &FoundOpt, &Operator, ErrorSubstring)) {
        return FALSE;
    }

    YoriLibInitEmptyString(&Value);

    Value.StartOfString = Operator.StartOfString + Operator.LengthInChars;
    Value.LengthInChars = FilterElement->LengthInChars - Operator.LengthInChars - (DWORD)(Operator.StartOfString - FilterElement->StartOfString);

    return YoriLibFileFiltParseFilterOperator(Criteria, &Operator, &Value, FoundOpt, ErrorSubstring);
}

/**
 Parse a single user supplied option string into a criteria to apply against
 each file found.  This function performs all string validation.

 @param Criteria Pointer to the criteria object to populate with the result
        of this function.  Note for this function this points to a color
        criteria describing how to determine a match and which color to
        apply if a match is found.

 @param FilterElement Pointer to the string describing a single thing to
        filter against.  This string should indicate the option, operator,
        comparison value, a comma, and the color to apply.  This function
        needs to perform all validation that these details are provided and
        are meaningful.  Note this string is not NULL terminated since it
        refers to a portion of the user's string.

 @param ErrorSubstring On failure, updated to point to the part of the user's
        expression that caused the failure.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibFileFiltParseColorElement(
    __out PYORI_LIB_FILE_FILT_MATCH_CRITERIA Criteria,
    __in PYORI_STRING FilterElement,
    __out _On_failure_(_Post_valid_) PYORI_STRING ErrorSubstring
    )
{
    YORI_STRING Operator;
    YORI_STRING Value;
    PYORI_LIB_FILE_FILT_COLOR_CRITERIA ColorCriteria = (PYORI_LIB_FILE_FILT_COLOR_CRITERIA)Criteria;
    PCYORI_LIB_FILE_FILT_FILTER_OPT FoundOpt;
    DWORD CharsToCompare;
    DWORD CharsRemaining;

    if (!YoriLibFileFiltParseFilterOptAndOperator(FilterElement, &FoundOpt, &Operator, ErrorSubstring)) {
        return FALSE;
    }

    YoriLibInitEmptyString(&Value);

    Value.StartOfString = Operator.StartOfString + Operator.LengthInChars;
    CharsRemaining = FilterElement->LengthInChars - Operator.LengthInChars - (DWORD)(Operator.StartOfString - FilterElement->StartOfString);
    Value.LengthInChars = CharsRemaining;

    CharsToCompare = YoriLibCountStringNotContainingChars(&Value, _T(","));
    if (Value.LengthInChars == CharsToCompare) {
        YoriLibInitEmptyString(ErrorSubstring);
        ErrorSubstring->StartOfString = FilterElement->StartOfString;
        ErrorSubstring->LengthInChars = FilterElement->LengthInChars;
        return FALSE;
    }

    Value.LengthInChars = CharsToCompare;

    if (!YoriLibFileFiltParseFilterOperator(Criteria, &Operator, &Value, FoundOpt, ErrorSubstring)) {
        return FALSE;
    }

    Value.StartOfString += CharsToCompare + 1;
    Value.LengthInChars = CharsRemaining - CharsToCompare - 1;

    YoriLibAttributeFromString(&Value, &ColorCriteria->Color);
    return TRUE;
}


/**
 A callback function which can be invoked to parse each element in a
 semicolon delimited list of filter rules to apply.
 */
typedef 
BOOL
YORI_LIB_FILE_FILT_PARSE_FN(PYORI_LIB_FILE_FILT_MATCH_CRITERIA, PYORI_STRING, PYORI_STRING);

/**
 A pointer to a callback function which can be invoked to parse each element
 in a semicolon delimited list of filter rules to apply.
 */
typedef YORI_LIB_FILE_FILT_PARSE_FN *PYORI_LIB_FILE_FILT_PARSE_FN;

/**
 Parse a complete user supplied filter string into a series of options and
 build a list of criteria to filter against.

 @param Filter Pointer to the filter object to populate with a list of one or
        more criteria to filter all files found against.

 @param FilterString Pointer to the string describing all of the filters to
        apply to all found files.

 @param Fn Pointer to a callback function to invoke for each element found.

 @param AllocationSize Specifies the size, in bytes, needed for each element
        generated.

 @param ErrorSubstring On failure, updated to point to the part of the user's
        expression that caused the failure.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibFileFiltParseFilterStringInternal(
    __out PYORI_LIB_FILE_FILTER Filter,
    __in PYORI_STRING FilterString,
    __in PYORI_LIB_FILE_FILT_PARSE_FN Fn,
    __in DWORD AllocationSize,
    __out _On_failure_(_Post_valid_) PYORI_STRING ErrorSubstring
    )
{
    YORI_STRING Remaining;
    YORI_STRING Element;
    PYORI_LIB_FILE_FILT_MATCH_CRITERIA Criteria = NULL;
    PYORI_LIB_FILE_FILT_MATCH_CRITERIA ThisElement;
    LPTSTR NextStart;
    DWORD ElementCount;
    DWORD Index;
    DWORD Phase;

    ASSERT(AllocationSize >= sizeof(YORI_LIB_FILE_FILT_MATCH_CRITERIA));

    YoriLibInitEmptyString(&Remaining);
    YoriLibInitEmptyString(&Element);
    YoriLibInitEmptyString(ErrorSubstring);

    ElementCount = 0;
    for (Phase = 0; Phase < 2; Phase++) {
        Remaining.StartOfString = FilterString->StartOfString;
        Remaining.LengthInChars = FilterString->LengthInChars;

        while (TRUE) {
            NextStart = YoriLibFindLeftMostCharacter(&Remaining, ';');
            Element.StartOfString = Remaining.StartOfString;
            if (NextStart != NULL) {
                Element.LengthInChars = (DWORD)(NextStart - Element.StartOfString);
            } else {
                Element.LengthInChars = Remaining.LengthInChars;
            }

            YoriLibTrimSpaces(&Element);
            if (Element.LengthInChars > 0) {
                if (Criteria != NULL) {
                    ASSERT(Phase == 1);
                    ThisElement = (PYORI_LIB_FILE_FILT_MATCH_CRITERIA)YoriLibAddToPointer(Criteria, ElementCount * AllocationSize);
                    if (!Fn(ThisElement, &Element, ErrorSubstring)) {
                        YoriLibFree(Criteria);
                        return FALSE;
                    }

                    //
                    //  At the expense of being N^2, check if a previous
                    //  item is already collecting the same data.  If it
                    //  is, don't collect anything by this item.  The
                    //  hope is this filter chain is executed across
                    //  multiple files so the cost of this check will be
                    //  outweighed by the operations it eliminates.
                    //

                    if (ThisElement->CollectFn != NULL) {
                        PYORI_LIB_FILE_FILT_MATCH_CRITERIA PreviousElement;
                        for (Index = 0; Index < ElementCount; Index++) {
                            PreviousElement = (PYORI_LIB_FILE_FILT_MATCH_CRITERIA)YoriLibAddToPointer(Criteria, Index * AllocationSize);
                            if (ThisElement->CollectFn == PreviousElement->CollectFn) {
                                ThisElement->CollectFn = NULL;
                                break;
                            }
                        }
                    }
                }
                ElementCount++;
            }

            if (NextStart == NULL) {
                break;
            }

            Remaining.StartOfString = NextStart;
            Remaining.LengthInChars = FilterString->LengthInChars - (DWORD)(NextStart - FilterString->StartOfString);

            if (Remaining.LengthInChars > 0) {
                Remaining.StartOfString++;
                Remaining.LengthInChars--;
            }

            if (Remaining.LengthInChars == 0) {
                break;
            }
        }

        if (Phase == 0) {
            Criteria = YoriLibMalloc(ElementCount * AllocationSize);
            if (Criteria == NULL) {
                YoriLibInitEmptyString(ErrorSubstring);
                return FALSE;
            }
            ZeroMemory(Criteria, ElementCount * AllocationSize);
            ElementCount = 0;
        }
    }

    Filter->Criteria = Criteria;
    Filter->ElementSize = AllocationSize;
    Filter->NumberCriteria = ElementCount;
    return TRUE;
}

/**
 Parse a string that consists of a semicolon delimited list of elements, with
 each element containing a criteria, operator and comparison value.

 @param Filter Pointer to the filter object to populate with the rules to 
        apply for each element.

 @param FilterString Pointer to the string containing the list to parse.

 @param ErrorSubstring If this function fails, this is populated with the
        range within the FilterString that was determined to be in error.
        Note this is not NULL terminated.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibFileFiltParseFilterString(
    __out PYORI_LIB_FILE_FILTER Filter,
    __in PYORI_STRING FilterString,
    __out _On_failure_(_Post_valid_) PYORI_STRING ErrorSubstring
    )
{
    return YoriLibFileFiltParseFilterStringInternal(Filter, FilterString, YoriLibFileFiltParseFilterElement, sizeof(YORI_LIB_FILE_FILT_MATCH_CRITERIA), ErrorSubstring);
}

/**
 Parse a string that consists of a semicolon delimited list of elements, with
 each element containing a criteria, operator, comparison value, and color to
 apply in event of a match.

 @param Filter Pointer to the filter object to populate with the rules to 
        apply for each element.

 @param ColorString Pointer to the string containing the list to parse.

 @param ErrorSubstring If this function fails, this is populated with the
        range within the ColorString that was determined to be in error.
        Note this is not NULL terminated.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibFileFiltParseColorString(
    __out PYORI_LIB_FILE_FILTER Filter,
    __in PYORI_STRING ColorString,
    __out _On_failure_(_Post_valid_) PYORI_STRING ErrorSubstring
    )
{
    return YoriLibFileFiltParseFilterStringInternal(Filter, ColorString, YoriLibFileFiltParseColorElement, sizeof(YORI_LIB_FILE_FILT_COLOR_CRITERIA), ErrorSubstring);
}

/**
 Evaluate whether a found file meets the criteria specified by the user
 supplied filter string.

 @param Filter Pointer to the filter object which contains a list of filters
        to apply.

 @param FilePath Pointer to a fully qualified file path.

 @param FileInfo Pointer to the information returned from directory
        enumeration.

 @return TRUE to indicate the file meets all of the filter criteria and
         should be included, FALSE to indicate the file has failed one or
         more criteria and should be excluded.
 */
__success(return)
BOOL
YoriLibFileFiltCheckFilterMatch(
    __in PYORI_LIB_FILE_FILTER Filter,
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo
    )
{
    DWORD Count;
    YORI_FILE_INFO CompareEntry;
    PYORI_LIB_FILE_FILT_MATCH_CRITERIA CriteriaArray;
    PYORI_LIB_FILE_FILT_MATCH_CRITERIA Criteria;
    
    if (Filter->NumberCriteria == 0) {
        return TRUE;
    }

    ZeroMemory(&CompareEntry, sizeof(CompareEntry));

    CriteriaArray = (PYORI_LIB_FILE_FILT_MATCH_CRITERIA)Filter->Criteria;
    for (Count = 0; Count < Filter->NumberCriteria; Count++) {
        Criteria = &CriteriaArray[Count];
        if (Criteria->CollectFn != NULL &&
            !Criteria->CollectFn(&CompareEntry, FileInfo, FilePath)) {

            return FALSE;
        }

        if (!Criteria->TruthStates[Criteria->CompareFn(&CompareEntry, &Criteria->CompareEntry)]) {
            return FALSE;
        }
    }

    return TRUE;
}

/**
 Evaluate which color a file should be displayed as based on the user
 supplied filter string.

 @param Filter Pointer to the filter object which contains a list of filters
        to apply.

 @param FilePath Pointer to a fully qualified file path.

 @param FileInfo Pointer to the information returned from directory
        enumeration.

 @param Attribute On successful completion, updated with the color to use to
        display the file.

 @return TRUE to indicate a color has been found, FALSE if no color has
         been determined.
 */
__success(return)
BOOL
YoriLibFileFiltCheckColorMatch(
    __in PYORI_LIB_FILE_FILTER Filter,
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __out PYORILIB_COLOR_ATTRIBUTES Attribute
    )
{
    DWORD Index;
    YORILIB_COLOR_ATTRIBUTES ThisAttribute;
    YORILIB_COLOR_ATTRIBUTES PreviousAttributes;
    PYORI_LIB_FILE_FILT_COLOR_CRITERIA ThisApply;
    PYORI_LIB_FILE_FILT_COLOR_CRITERIA ColorsToApply;
    YORI_FILE_INFO CompareEntry;

    ZeroMemory(&CompareEntry, sizeof(CompareEntry));

    ThisAttribute.Ctrl = YORILIB_ATTRCTRL_WINDOW_BG | YORILIB_ATTRCTRL_WINDOW_FG;
    ThisAttribute.Win32Attr = 0;

    PreviousAttributes.Ctrl = YORILIB_ATTRCTRL_WINDOW_BG | YORILIB_ATTRCTRL_WINDOW_FG;
    PreviousAttributes.Win32Attr = (UCHAR)YoriLibVtGetDefaultColor();

    //
    //  We expect each element to be the criteria determining a match and
    //  color to apply in event of a match.
    //

    ASSERT((Filter->ElementSize == 0 &&
            Filter->NumberCriteria == 0) ||
           Filter->ElementSize == sizeof(YORI_LIB_FILE_FILT_COLOR_CRITERIA));

    ColorsToApply = (PYORI_LIB_FILE_FILT_COLOR_CRITERIA)Filter->Criteria;
    for (Index = 0; Index < Filter->NumberCriteria; Index++) {
        ThisApply = &ColorsToApply[Index];

        if (ThisApply->Match.CollectFn != NULL &&
            !ThisApply->Match.CollectFn(&CompareEntry, FileInfo, FilePath)) {

            return FALSE;
        }

        if (ThisApply->Match.TruthStates[ThisApply->Match.CompareFn(&CompareEntry, &ThisApply->Match.CompareEntry)]) {
            YoriLibCombineColors(ThisAttribute, ThisApply->Color, &ThisAttribute);
            if ((ThisAttribute.Ctrl & YORILIB_ATTRCTRL_CONTINUE) == 0) {

                YoriLibResolveWindowColorComponents(ThisAttribute, PreviousAttributes, TRUE, &ThisAttribute);

                if (ThisAttribute.Ctrl & YORILIB_ATTRCTRL_INVERT) {
                    ThisAttribute.Win32Attr = (UCHAR)(((ThisAttribute.Win32Attr & 0x0F) << 4) + ((ThisAttribute.Win32Attr & 0xF0) >> 4));
                    ThisAttribute.Ctrl = (UCHAR)(ThisAttribute.Ctrl & ~(YORILIB_ATTRCTRL_INVERT));
                }

                Attribute->Ctrl = ThisAttribute.Ctrl;
                Attribute->Win32Attr = ThisAttribute.Win32Attr;
                return TRUE;
            }

            ThisAttribute.Ctrl = (UCHAR)(ThisAttribute.Ctrl & ~(YORILIB_ATTRCTRL_CONTINUE));
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

    Attribute->Ctrl = PreviousAttributes.Ctrl;
    Attribute->Win32Attr = PreviousAttributes.Win32Attr;
    return TRUE;
}

/**
 Deallocate any memory associated with a file filter.  Note the structure
 itself is not deallocated since it is typically on the stack or embedded in
 another structure.

 @param Filter Pointer to the filter object to deallocate.
 */
VOID
YoriLibFileFiltFreeFilter(
    __in PYORI_LIB_FILE_FILTER Filter
    )
{
    if (Filter->Criteria != NULL) {
        YoriLibFree(Filter->Criteria);
    }
    Filter->Criteria = NULL;
    Filter->NumberCriteria = 0;
}

// vim:sw=4:ts=4:et:
