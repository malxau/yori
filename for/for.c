/**
 * @file for/for.c
 *
 * Yori shell enumerate and operate on strings or files
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

#include <yoripch.h>
#include <yorilib.h>
#ifdef YORI_BUILTIN
#include <yoricall.h>
#endif

/**
 Help text to display to the user.
 */
const
CHAR strForHelpText[] =
        "Enumerates through a list of strings or files.\n"
        "\n"
        "FOR [-license] [-b] [-c] [-d] [-p n] <var> in (<list>) do <cmd>\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -c             Use cmd as a subshell rather than Yori\n"
        "   -d             Match directories rather than files\n"
        "   -i <criteria>  Only treat match files if they meet criteria, see below"
        "   -l             Use (start,step,end) notation for the list\n"
        "   -p <n>         Execute with <n> concurrent processes\n"
        "\n"
        " The -i option will match files only if they meet criteria.  This is a\n"
        " semicolon delimited list of entries matching the following form:\n"
        "\n"
        "   [file attribute][operator][criteria]\n"
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
 Specifies a pointer to a function which can compare two directory entries
 in some fashion.
 */
typedef DWORD (* FOR_COMPARE_FN)(PYORI_FILE_INFO, PYORI_FILE_INFO);

/**
 Specifies a pointer to a function which can collect file information from
 the disk or file system for some particular piece of data.
 */
typedef BOOL (* FOR_COLLECT_FN)(PYORI_FILE_INFO, PWIN32_FIND_DATA, PYORI_STRING);

/**
 Specifies a pointer to a function which can generate in memory file
 information from a user provided string.
 */
typedef BOOL (* FOR_GENERATE_FROM_STRING_FN)(PYORI_FILE_INFO, PYORI_STRING);

/**
 A single option that files can be filtered against.
 */
typedef struct _FOR_FILTER_OPT {

    /**
     The two character switch name for the option and a NULL terminator.
     */
    TCHAR Switch[3];

    /**
     Pointer to a function to collect the data from a specific file for the
     option.
     */
    FOR_COLLECT_FN CollectFn;

    /**
     Pointer to a function to compare the user supplied value against the
     value from a given file.
     */
    FOR_COMPARE_FN CompareFn;

    /**
     Pointer to a function to compare the user supplied value against the
     value from a given file in a bitwise fashion.
     */
    FOR_COMPARE_FN BitwiseCompareFn;

    /**
     Pointer to a function to convert the user supplied string into a value
     for the option.
     */
    FOR_GENERATE_FROM_STRING_FN GenerateFromStringFn;

    /**
     A string containing a description for the option.
     */
    CHAR Help[24];
} FOR_FILTER_OPT, *PFOR_FILTER_OPT;

/**
 A pointer to a filter option that cannot change.  Because options are
 compiled into the binary in read only form, all pointers should really
 be const pointers.
 */
typedef FOR_FILTER_OPT CONST * PCFOR_FILTER_OPT;

/**
 An array of options that are supported by this program.
 */
CONST FOR_FILTER_OPT
ForFilterOptions[] = {
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

    /*
    {_T("ep"),                               YoriLibCollectEffectivePermissions,
     YoriLibCompareEffectivePermissions,     YoriLibBitwiseEffectivePermissions,
     YoriLibGenerateEffectivePermissions,    "effective permissions"},

    {_T("fa"),                               YoriLibCollectFileAttributes,
     YoriLibCompareFileAttributes,           YoriLibBitwiseFileAttributes,
     YoriLibGenerateFileAttributes,          "file attributes"},
     */

    {_T("fc"),                               YoriLibCollectFragmentCount,
     YoriLibCompareFragmentCount,            NULL,
     YoriLibGenerateFragmentCount,           "fragment count"},

    {_T("fe"),                               YoriLibCollectFileExtension,
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
ForHelp()
{
    DWORD Index;

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("For %i.%i\n"), FOR_VER_MAJOR, FOR_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strForHelpText);

    //
    //  Display supported options and operators
    //

    for (Index = 0; Index < sizeof(ForFilterOptions)/sizeof(ForFilterOptions[0]); Index++) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, 
                      _T("   %s (%hs), %hs%hs%hs\n"),
                      ForFilterOptions[Index].Switch,
                      ForFilterOptions[Index].Help,
                      ForFilterOptions[Index].CompareFn?"=, !=, >, >=, <, <=":"",
                      (ForFilterOptions[Index].CompareFn && ForFilterOptions[Index].BitwiseCompareFn)?", ":"",
                      ForFilterOptions[Index].BitwiseCompareFn?"&, !&":"");

    }
    return TRUE;
}


/**
 An in memory representation of a single match criteria, specifying the color
 to apply in event that the incoming file matches a specified criteria.
 */
typedef struct _FOR_MATCH_CRITERIA {

    /**
     Pointer to a function to ingest an incoming directory entry so that we
     have two objects to compare against.
     */
    FOR_COLLECT_FN CollectFn;

    /**
     Pointer to a function to compare an incoming directory entry against the
     dummy one contained here.
     */
    FOR_COMPARE_FN CompareFn;

    /**
     An array indicating whether a match is found if the comparison returns
     less than, greater than, or equal.
     */
    BOOL TruthStates[3];

    /**
     A dummy directory entry containing values to compare against.  This is
     used to allow all compare functions to operate on two directory entries.
     */
    YORI_FILE_INFO CompareEntry;
} FOR_MATCH_CRITERIA, *PFOR_MATCH_CRITERIA;

/**
 State about the currently running processes as well as information required
 to launch any new processes from this program.
 */
typedef struct _FOR_EXEC_CONTEXT {

    /**
     If TRUE, use CMD as a subshell.  If FALSE, use Yori.
     */
    BOOL InvokeCmd;

    /**
     The string that might be found in ArgV which should be changed to contain
     the value of any match.
     */
    PYORI_STRING SubstituteVariable;

    /**
     The number of elements in ArgV.
     */
    DWORD ArgC;

    /**
     The template form of an argv style argument array, before any
     substitution has taken place.
     */
    PYORI_STRING ArgV;

    /**
     The number of processes that this program would like to have concurrently
     running.
     */
    DWORD TargetConcurrentCount;

    /**
     The number of processes that are currently running as a result of this
     program.
     */
    DWORD CurrentConcurrentCount;

    /**
     An array of handles with CurrentConcurrentCount number of valid
     elements.  These correspond to processes that are currently running.
     */
    PHANDLE HandleArray;

    /**
     A pointer to a list of criteria to filter matches against.
     */
    PFOR_MATCH_CRITERIA Filter;

    /**
     The number of elements in the Filter array.
     */
    DWORD FilterCount;

} FOR_EXEC_CONTEXT, *PFOR_EXEC_CONTEXT;


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

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
ForParseFilterOperator(
    __inout PFOR_MATCH_CRITERIA Criteria,
    __in PYORI_STRING Operator,
    __in PYORI_STRING Value,
    __in PCFOR_FILTER_OPT MatchedOption
    )
{
    //
    //  Based on the operator, fill in the truth table.  We'll
    //  use the generic compare function and based on this
    //  truth table we'll decide whether the rule is satisfied
    //  or not.
    //

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
        return FALSE;
    }

    //
    //  Maybe the operator specified was valid but not supported
    //  by this type.
    //

    if (Criteria->CompareFn == NULL) {
        return FALSE;
    }

    Criteria->CollectFn = MatchedOption->CollectFn;

    //
    //  If we fail to capture this, ignore it and move on to the
    //  next entry, clobbering this structure again.
    //

    if (!MatchedOption->GenerateFromStringFn(&Criteria->CompareEntry, Value)) {
        return FALSE;
    }

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

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
ForParseFilterElement(
    __out PFOR_MATCH_CRITERIA Criteria,
    __in PYORI_STRING FilterElement
    )
{
    YORI_STRING Operator;
    YORI_STRING Value;
    YORI_STRING SwitchName;
    DWORD Count;
    PCFOR_FILTER_OPT FoundOpt;

    YoriLibInitEmptyString(&SwitchName);
    SwitchName.StartOfString = FilterElement->StartOfString;
    SwitchName.LengthInChars = FilterElement->LengthInChars;


    YoriLibTrimSpaces(&SwitchName);

    SwitchName.LengthInChars = YoriLibCountStringNotContainingChars(&SwitchName, _T("&<>=!"));
    FoundOpt = NULL;

    for (Count = 0; Count < sizeof(ForFilterOptions)/sizeof(ForFilterOptions[0]); Count++) {
        if (YoriLibCompareStringWithLiteralInsensitive(&SwitchName, ForFilterOptions[Count].Switch) == 0) {
            FoundOpt = &ForFilterOptions[Count];
            break;
        }
    }

    if (FoundOpt == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Unknown criteria: %y\n"), &SwitchName);
        return FALSE;
    }

    YoriLibInitEmptyString(&Operator);

    Operator.StartOfString = SwitchName.StartOfString + SwitchName.LengthInChars;
    Operator.LengthInChars = FilterElement->LengthInChars - SwitchName.LengthInChars - (DWORD)(SwitchName.StartOfString - FilterElement->StartOfString);

    Operator.LengthInChars = YoriLibCountStringContainingChars(&Operator, _T("&<>=!"));

    YoriLibInitEmptyString(&Value);

    Value.StartOfString = Operator.StartOfString + Operator.LengthInChars;
    Value.LengthInChars = FilterElement->LengthInChars - Operator.LengthInChars - (DWORD)(Operator.StartOfString - FilterElement->StartOfString);

    return ForParseFilterOperator(Criteria, &Operator, &Value, FoundOpt);
}

/**
 Parse a complete user supplied filter string into a series of options and
 build a list of criteria to filter against.

 @param ExecContext Pointer to the exec context to populate with a list of
        one or more criteria to filter all files found against.

 @param FilterString Pointer to the string describing all of the filters to
        apply to all found files.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
ForParseFilterString(
    __inout PFOR_EXEC_CONTEXT ExecContext,
    __in PYORI_STRING FilterString
    )
{
    YORI_STRING Remaining;
    YORI_STRING Element;
    PFOR_MATCH_CRITERIA Criteria = NULL;
    LPTSTR NextStart;
    DWORD ElementCount;
    DWORD Phase;

    YoriLibInitEmptyString(&Remaining);
    YoriLibInitEmptyString(&Element);

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
                    if (!ForParseFilterElement(&Criteria[ElementCount], &Element)) {
                        YoriLibFree(Criteria);
                        return FALSE;
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
            Criteria = YoriLibMalloc(ElementCount * sizeof(FOR_MATCH_CRITERIA));
            if (Criteria == NULL) {
                return FALSE;
            }
            ZeroMemory(Criteria, ElementCount * sizeof(FOR_MATCH_CRITERIA));
            ElementCount = 0;
        }
    }

    ExecContext->Filter = Criteria;
    ExecContext->FilterCount = ElementCount;
    return TRUE;
}

/**
 Evaluate whether a found file meets the criteria specified by the user
 supplied filter string.

 @param ExecContext Pointer to the exec context which contains a list of
        filters to apply.

 @param FilePath Pointer to a fully qualified file path.

 @param FileInfo Pointer to the information returned from directory
        enumeration.

 @return TRUE to indicate the file meets all of the filter criteria and
         should be included, FALSE to indicate the file has failed one or
         more criteria and should be excluded.
 */
BOOL
ForCheckFilterMatch(
    __in PFOR_EXEC_CONTEXT ExecContext,
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo
    )
{
    DWORD Count;
    YORI_FILE_INFO CompareEntry;
    PFOR_MATCH_CRITERIA Criteria;
    
    if (ExecContext->FilterCount == 0) {
        return TRUE;
    }

    ZeroMemory(&CompareEntry, sizeof(CompareEntry));

    for (Count = 0; Count < ExecContext->FilterCount; Count++) {
        Criteria = &ExecContext->Filter[Count];
        if (!Criteria->CollectFn(&CompareEntry, FileInfo, FilePath)) {
            return FALSE;
        }

        if (!Criteria->TruthStates[Criteria->CompareFn(&CompareEntry, &Criteria->CompareEntry)]) {
            return FALSE;
        }
    }

    return TRUE;
}

/**
 Wait for any single process to complete.

 @param ExecContext Pointer to the for exec context containing information
        about currently running processes.
 */
VOID
ForWaitForProcessToComplete(
    __in PFOR_EXEC_CONTEXT ExecContext
    )
{
    DWORD Result;
    DWORD Index;
    DWORD Count;

    Result = WaitForMultipleObjects(ExecContext->CurrentConcurrentCount, ExecContext->HandleArray, FALSE, INFINITE);

    ASSERT(Result >= WAIT_OBJECT_0 && Result < (WAIT_OBJECT_0 + ExecContext->CurrentConcurrentCount));

    Index = Result - WAIT_OBJECT_0;

    CloseHandle(ExecContext->HandleArray[Index]);

    for (Count = Index; Count < ExecContext->CurrentConcurrentCount - 1; Count++) {
        ExecContext->HandleArray[Count] = ExecContext->HandleArray[Count + 1];
    }

    ExecContext->CurrentConcurrentCount--;
}

/**
 Execute a new command in response to a newly matched element.

 @param Match The match that was found from the set.

 @param ExecContext The current state of child processes and information about
        the arguments for any new child process.
 */
VOID
ForExecuteCommand(
    __in PYORI_STRING Match,
    __in PFOR_EXEC_CONTEXT ExecContext
    )
{
    DWORD ArgsNeeded = ExecContext->ArgC;
    DWORD PrefixArgCount;
    DWORD FoundOffset;
    DWORD Count;
    DWORD SubstitutesFound;
    DWORD ArgLengthNeeded;
    YORI_STRING OldArg;
    YORI_STRING NewArgWritePoint;
    PYORI_STRING NewArgArray;
    YORI_STRING CmdLine;
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFO StartupInfo;

    YoriLibInitEmptyString(&CmdLine);

#ifdef YORI_BUILTIN
    if (!ExecContext->InvokeCmd &&
        ExecContext->TargetConcurrentCount == 1) {
        PrefixArgCount = 0;
    } else {
        PrefixArgCount = 2;
    }
#else
    PrefixArgCount = 2;
#endif

    ArgsNeeded += PrefixArgCount;

    NewArgArray = YoriLibMalloc(ArgsNeeded * sizeof(YORI_STRING));
    if (NewArgArray == NULL) {
        return;
    }

    ZeroMemory(NewArgArray, ArgsNeeded * sizeof(YORI_STRING));

    //
    //  If a full path is specified in the environment, use it.  If not,
    //  use a file name only and let PATH evaluation find an interpreter.
    //

    if (PrefixArgCount > 0) {
        if (ExecContext->InvokeCmd) {
            ArgLengthNeeded = GetEnvironmentVariable(_T("COMSPEC"), NULL, 0);
            if (ArgLengthNeeded != 0) {
                if (YoriLibAllocateString(&NewArgArray[0], ArgLengthNeeded)) {
                    NewArgArray[0].LengthInChars = GetEnvironmentVariable(_T("COMSPEC"), NewArgArray[0].StartOfString, NewArgArray[0].LengthAllocated);
                    if (NewArgArray[0].LengthInChars == 0) {
                        YoriLibFreeStringContents(&NewArgArray[0]);
                    }
                }
            }
            if (NewArgArray[0].LengthInChars == 0) {
                YoriLibConstantString(&NewArgArray[0], _T("cmd.exe"));
            }
        } else {
            ArgLengthNeeded = GetEnvironmentVariable(_T("YORISPEC"), NULL, 0);
            if (ArgLengthNeeded != 0) {
                if (YoriLibAllocateString(&NewArgArray[0], ArgLengthNeeded)) {
                    NewArgArray[0].LengthInChars = GetEnvironmentVariable(_T("YORISPEC"), NewArgArray[0].StartOfString, NewArgArray[0].LengthAllocated);
                    if (NewArgArray[0].LengthInChars == 0) {
                        YoriLibFreeStringContents(&NewArgArray[0]);
                    }
                }
            }
            if (NewArgArray[0].LengthInChars == 0) {
                YoriLibConstantString(&NewArgArray[0], _T("yori.exe"));
            }
        }
        YoriLibConstantString(&NewArgArray[1], _T("/c"));
    }

    for (Count = 0; Count < ExecContext->ArgC; Count++) {
        YoriLibInitEmptyString(&OldArg);
        OldArg.StartOfString = ExecContext->ArgV[Count].StartOfString;
        OldArg.LengthInChars = ExecContext->ArgV[Count].LengthInChars;
        SubstitutesFound = 0;

        while (YoriLibFindFirstMatchingSubstring(&OldArg, 1, ExecContext->SubstituteVariable, &FoundOffset)) {
            SubstitutesFound++;
            OldArg.StartOfString += FoundOffset + 1;
            OldArg.LengthInChars -= FoundOffset + 1;
        }

        ArgLengthNeeded = ExecContext->ArgV[Count].LengthInChars + SubstitutesFound * Match->LengthInChars - SubstitutesFound * ExecContext->SubstituteVariable->LengthInChars + 1;
        if (!YoriLibAllocateString(&NewArgArray[Count + PrefixArgCount], ArgLengthNeeded)) {
            goto Cleanup;
        }

        YoriLibInitEmptyString(&NewArgWritePoint);
        NewArgWritePoint.StartOfString = NewArgArray[Count + PrefixArgCount].StartOfString;
        NewArgWritePoint.LengthAllocated = NewArgArray[Count + PrefixArgCount].LengthAllocated;

        YoriLibInitEmptyString(&OldArg);
        OldArg.StartOfString = ExecContext->ArgV[Count].StartOfString;
        OldArg.LengthInChars = ExecContext->ArgV[Count].LengthInChars;

        while (TRUE) {
            if (YoriLibFindFirstMatchingSubstring(&OldArg, 1, ExecContext->SubstituteVariable, &FoundOffset)) {
                memcpy(NewArgWritePoint.StartOfString, OldArg.StartOfString, FoundOffset * sizeof(TCHAR));
                NewArgWritePoint.StartOfString += FoundOffset;
                NewArgWritePoint.LengthAllocated -= FoundOffset;
                memcpy(NewArgWritePoint.StartOfString, Match->StartOfString, Match->LengthInChars * sizeof(TCHAR));
                NewArgWritePoint.StartOfString += Match->LengthInChars;
                NewArgWritePoint.LengthAllocated -= Match->LengthInChars;

                OldArg.StartOfString += FoundOffset + ExecContext->SubstituteVariable->LengthInChars;
                OldArg.LengthInChars -= FoundOffset + ExecContext->SubstituteVariable->LengthInChars;
            } else {
                memcpy(NewArgWritePoint.StartOfString, OldArg.StartOfString, OldArg.LengthInChars * sizeof(TCHAR));
                NewArgWritePoint.StartOfString += OldArg.LengthInChars;
                NewArgWritePoint.LengthAllocated -= OldArg.LengthInChars;
                NewArgWritePoint.StartOfString[0] = '\0';
                
                NewArgArray[Count + PrefixArgCount].LengthInChars = (DWORD)(NewArgWritePoint.StartOfString - NewArgArray[Count + PrefixArgCount].StartOfString);
                ASSERT(NewArgArray[Count + PrefixArgCount].LengthInChars < NewArgArray[Count + PrefixArgCount].LengthAllocated);
                ASSERT(YoriLibIsStringNullTerminated(&NewArgArray[Count + PrefixArgCount]));
                break;
            }
        }
    }

    if (!YoriLibBuildCmdlineFromArgcArgv(ArgsNeeded, NewArgArray, TRUE, &CmdLine)) {
        goto Cleanup;
    }

#ifdef YORI_BUILTIN
    if (PrefixArgCount == 0) {
        YoriCallExecuteExpression(&CmdLine);
        goto Cleanup;
    }
#endif

    memset(&StartupInfo, 0, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    if (!CreateProcess(NULL, CmdLine.StartOfString, NULL, NULL, TRUE, 0, NULL, NULL, &StartupInfo, &ProcessInfo)) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: execution failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        goto Cleanup;
    }

    CloseHandle(ProcessInfo.hThread);

    ExecContext->HandleArray[ExecContext->CurrentConcurrentCount] = ProcessInfo.hProcess;
    ExecContext->CurrentConcurrentCount++;

    if (ExecContext->CurrentConcurrentCount == ExecContext->TargetConcurrentCount) {
        ForWaitForProcessToComplete(ExecContext);
    }

Cleanup:

    for (Count = 0; Count < ArgsNeeded; Count++) {
        YoriLibFreeStringContents(&NewArgArray[Count]);
    }

    YoriLibFreeStringContents(&CmdLine);
    YoriLibFree(NewArgArray);
}

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Recursion depth, ignored in this application.

 @param ExecContext The current state of the program and information needed to
        launch new child processes.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
ForFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PFOR_EXEC_CONTEXT ExecContext
    )
{
    UNREFERENCED_PARAMETER(Depth);
    UNREFERENCED_PARAMETER(FileInfo);

    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    if (!ForCheckFilterMatch(ExecContext, FilePath, FileInfo)) {
        return TRUE;
    }

    ForExecuteCommand(FilePath, ExecContext);
    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the for builtin command.
 */
#define ENTRYPOINT YoriCmd_FOR
#else
/**
 The main entrypoint for the for standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the for cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    YORI_STRING Arg;
    BOOL LeftBraceOpen;
    BOOL RequiresExpansion;
    BOOL MatchDirectories;
    BOOL Recurse;
    BOOL BasicEnumeration;
    BOOL StepMode;
    DWORD CharIndex;
    DWORD MatchFlags;
    DWORD StartArg = 0;
    DWORD ListArg = 0;
    DWORD CmdArg = 0;
    DWORD ArgIndex;
    DWORD i;
    FOR_EXEC_CONTEXT ExecContext;

    ZeroMemory(&ExecContext, sizeof(ExecContext));

    ExecContext.TargetConcurrentCount = 1;
    ExecContext.CurrentConcurrentCount = 0;
    MatchDirectories = FALSE;
    Recurse = FALSE;
    StepMode = FALSE;
    BasicEnumeration = FALSE;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                ForHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                ExecContext.InvokeCmd = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("d")) == 0) {
                MatchDirectories = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("i")) == 0) {
                if (i + 1 < ArgC) {
                    
                    if (!ForParseFilterString(&ExecContext, &ArgV[i + 1])) {
                        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: error parsing filter string %y\n"), &ArgV[i + 1]);
                        goto cleanup_and_exit;
                    }
                    i++;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("l")) == 0) {
                StepMode = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("p")) == 0) {
                if (i + 1 < ArgC) {
                    LONGLONG LlNumberProcesses = 0;
                    DWORD CharsConsumed = 0;
                    YoriLibStringToNumber(&ArgV[i + 1], TRUE, &LlNumberProcesses, &CharsConsumed);
                    ExecContext.TargetConcurrentCount = (DWORD)LlNumberProcesses;
                    ArgumentUnderstood = TRUE;
                    if (ExecContext.TargetConcurrentCount < 1) {
                        ExecContext.TargetConcurrentCount = 1;
                    }
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("r")) == 0) {
                Recurse = TRUE;
                ArgumentUnderstood = TRUE;
            }
        } else {
            ArgumentUnderstood = TRUE;
            StartArg = i;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: missing argument\n"));
        goto cleanup_and_exit;
    }

    ExecContext.SubstituteVariable = &ArgV[StartArg];

    //
    //  We need at least "%i in (*) do cmd"
    //

    if (ArgC < StartArg + 4) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: missing argument\n"));
        goto cleanup_and_exit;
    }

    if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[StartArg + 1], _T("in")) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: 'in' not found\n"));
        goto cleanup_and_exit;
    }

    if (YoriLibCompareStringWithLiteralInsensitiveCount(&ArgV[StartArg + 2], _T("("), 1) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: left bracket not found\n"));
        goto cleanup_and_exit;
    }

    LeftBraceOpen = TRUE;

    //
    //  Walk through all arguments looking for a closing brace, then looking
    //  for "do".  Once we've done finding both, we have a trailing command
    //  string
    //

    ListArg = StartArg + 2;
    for (ArgIndex = ListArg; ArgIndex < ArgC; ArgIndex++) {
        if (LeftBraceOpen) {
            if (ArgV[ArgIndex].LengthInChars > 0 && ArgV[ArgIndex].StartOfString[ArgV[ArgIndex].LengthInChars - 1] == ')') {
                LeftBraceOpen = FALSE;
            }
        } else {
            if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[ArgIndex], _T("do")) == 0) {
                CmdArg = ArgIndex + 1;
                break;
            }
        }
    }

    if (CmdArg == 0) {
        if (LeftBraceOpen) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: right bracket not found\n"));
            goto cleanup_and_exit;
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: do not found\n"));
            goto cleanup_and_exit;
        }
    }

    if (CmdArg >= ArgC) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: command not found\n"));
        goto cleanup_and_exit;
    }

    ExecContext.ArgC = ArgC - CmdArg;
    ExecContext.ArgV = &ArgV[CmdArg];
    ExecContext.HandleArray = YoriLibMalloc(ExecContext.TargetConcurrentCount * sizeof(HANDLE));
    if (ExecContext.HandleArray == NULL) {
        goto cleanup_and_exit;
    }

    MatchFlags = 0;
    if (MatchDirectories) {
        MatchFlags = YORILIB_FILEENUM_RETURN_DIRECTORIES;
    } else {
        MatchFlags = YORILIB_FILEENUM_RETURN_FILES;
    }

    if (Recurse) {
        MatchFlags |= YORILIB_FILEENUM_RECURSE_AFTER_RETURN | YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;
    }
    if (BasicEnumeration) {
        MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
    }

    if (StepMode) {
        LONGLONG Start;
        LONGLONG Step;
        LONGLONG End;
        LONGLONG Current;
        DWORD CharsConsumed;
        YORI_STRING FoundMatch;
        YORI_STRING Criteria;

        if (!YoriLibBuildCmdlineFromArgcArgv(CmdArg - ListArg, &ArgV[ListArg], FALSE, &Criteria)) {
            goto cleanup_and_exit;
        }

        //
        //  Remove the brackets
        //

        Criteria.StartOfString++;
        Criteria.LengthInChars -= 2;
        YoriLibTrimSpaces(&Criteria);

        //
        //  Get the start value
        //

        if (!YoriLibStringToNumber(&Criteria, FALSE, &Start, &CharsConsumed)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: argument not numeric\n"));
            YoriLibFreeStringContents(&Criteria);
            goto cleanup_and_exit;
        }

        Criteria.StartOfString += CharsConsumed;
        Criteria.LengthInChars -= CharsConsumed;
        YoriLibTrimSpaces(&Criteria);

        if (Criteria.StartOfString[0] != ',') {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: seperator not found\n"));

            YoriLibFreeStringContents(&Criteria);
            goto cleanup_and_exit;
        }
        
        Criteria.StartOfString += 1;
        Criteria.LengthInChars -= 1;
        YoriLibTrimSpaces(&Criteria);

        //
        //  Get the step value
        //

        if (!YoriLibStringToNumber(&Criteria, FALSE, &Step, &CharsConsumed)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: argument not numeric\n"));
            YoriLibFreeStringContents(&Criteria);
            goto cleanup_and_exit;
        }

        Criteria.StartOfString += CharsConsumed;
        Criteria.LengthInChars -= CharsConsumed;
        YoriLibTrimSpaces(&Criteria);

        if (Criteria.StartOfString[0] != ',') {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: seperator not found\n"));

            YoriLibFreeStringContents(&Criteria);
            goto cleanup_and_exit;
        }
        
        Criteria.StartOfString += 1;
        Criteria.LengthInChars -= 1;
        YoriLibTrimSpaces(&Criteria);

        //
        //  Get the end value
        //

        if (!YoriLibStringToNumber(&Criteria, FALSE, &End, &CharsConsumed)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: argument not numeric\n"));
            YoriLibFreeStringContents(&Criteria);
            goto cleanup_and_exit;
        }

        YoriLibFreeStringContents(&Criteria);

        if (!YoriLibAllocateString(&FoundMatch, 32)) {
            goto cleanup_and_exit;
        }

        Current = Start;

        do {
            if (Step > 0) {
                if (Current > End) {
                    break;
                }
            } else if (Step < 0) {
                if (Current < End) {
                    break;
                }
            }

            YoriLibNumberToString(&FoundMatch, Current, 10, 0, '\0');
            ForExecuteCommand(&FoundMatch, &ExecContext);
            Current += Step;
        } while(TRUE);

        YoriLibFreeStringContents(&FoundMatch);

    } else {

        for (ArgIndex = ListArg; ArgIndex < CmdArg - 1; ArgIndex++) {
            YORI_STRING ThisMatch;

            YoriLibInitEmptyString(&ThisMatch);

            ThisMatch.StartOfString = ArgV[ArgIndex].StartOfString;
            ThisMatch.LengthInChars = ArgV[ArgIndex].LengthInChars;
            ThisMatch.LengthAllocated = ArgV[ArgIndex].LengthAllocated;

            if (ArgIndex == ListArg) {
                ThisMatch.StartOfString++;
                ThisMatch.LengthInChars--;
                ThisMatch.LengthAllocated--;
            }

            if (ArgIndex == CmdArg - 2) {
                LPTSTR NullTerminatedString;

                ThisMatch.LengthInChars--;
                ThisMatch.LengthAllocated--;
                NullTerminatedString = YoriLibCStringFromYoriString(&ThisMatch);
                if (NullTerminatedString == NULL) {
                    goto cleanup_and_exit;
                }
                ThisMatch.MemoryToFree = NullTerminatedString;
                ThisMatch.StartOfString = NullTerminatedString;
            }

            RequiresExpansion = FALSE;
            for (CharIndex = 0; CharIndex < ThisMatch.LengthInChars; CharIndex++) {
                if (ThisMatch.StartOfString[CharIndex] == '*' ||
                    ThisMatch.StartOfString[CharIndex] == '?') {

                    RequiresExpansion = TRUE;
                    break;
                }
                if (!BasicEnumeration) {
                    if (ThisMatch.StartOfString[CharIndex] == '[' ||
                        ThisMatch.StartOfString[CharIndex] == '{') {
    
                        RequiresExpansion = TRUE;
                        break;
                    }
                }
            }

            if (ThisMatch.LengthInChars > 0) {

                if (RequiresExpansion) {
                    YoriLibForEachFile(&ThisMatch, MatchFlags, 0, ForFileFoundCallback, &ExecContext);
                } else {
                    ForExecuteCommand(&ThisMatch, &ExecContext);
                }
            }

            //
            //  Because MemoryToFree is not normally populated, this only
            //  really frees where the memory was double buffered
            //

            YoriLibFreeStringContents(&ThisMatch);
        }
    }

    while (ExecContext.CurrentConcurrentCount > 0) {
        ForWaitForProcessToComplete(&ExecContext);
    }

    if (ExecContext.Filter) {
        YoriLibFree(ExecContext.Filter);
    }
    YoriLibFree(ExecContext.HandleArray);

    return EXIT_SUCCESS;

cleanup_and_exit:

    if (ExecContext.Filter) {
        YoriLibFree(ExecContext.Filter);
    }

    return EXIT_FAILURE;
}

// vim:sw=4:ts=4:et:
