/**
 * @file builtins/job.c
 *
 * Yori shell job management
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
#include <yoricall.h>

/**
 Help text to display to the user.
 */
const
CHAR strJobHelpText[] =
        "\n"
        "Displays or updates background job status.\n"
        "\n"
        "JOB [-license]\n"
        "JOB ERRORS <id>\n"
        "JOB EXITCODE <id>\n"
        "JOB KILL <id>\n"
        "JOB NICE <id>\n"
        "JOB OUTPUT <id>\n";

/**
 Display usage text to the user.
 */
BOOL
JobHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Job %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strJobHelpText);
    return TRUE;
}

/**
 Builtin command for managing background jobs.

 @param ArgC The number of arguments.

 @param ArgV Pointer to the array of arguments.

 @return Zero for success, nonzero on failure.
 */
DWORD
YORI_BUILTIN_FN
YoriCmd_JOB(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD JobId = 0;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 0;
    YORI_STRING Arg;
    YORI_MAX_SIGNED_T llTemp;
    YORI_ALLOC_SIZE_T CharsConsumed;

    YoriLibLoadNtDllFunctions();
    YoriLibLoadKernel32Functions();

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                JobHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
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

    llTemp = 0;

    if (ArgC <= 1) {
        JobId = YoriCallGetNextJobId(JobId);
        while (JobId != 0) {
            BOOL HasCompleted;
            BOOL HasOutput;
            DWORD ExitCode;
            YORI_STRING Command;
            HasCompleted = FALSE;
            HasOutput = FALSE;

            YoriLibInitEmptyString(&Command);

            if (YoriCallGetJobInformation(JobId, &HasCompleted, &HasOutput, &ExitCode, &Command)) {
                if (HasCompleted && HasOutput) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Job %i (completed, output available): %y\n"), JobId, &Command);
                } else if (HasCompleted) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Job %i (completed): %y\n"), JobId, &Command);
                } else {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Job %i (executing): %y\n"), JobId, &Command);
                }

                YoriCallFreeYoriString(&Command);
            }
            JobId = YoriCallGetNextJobId(JobId);
        }

    } else {

        if (YoriLibCompareStringLitIns(&ArgV[1], _T("errors")) == 0) {
            YORI_STRING Output;
            YORI_STRING Errors;
            if (ArgC < 3) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Job not specified\n"));
                return EXIT_FAILURE;
            }
            if (!YoriLibStringToNumber(&ArgV[2], TRUE, &llTemp, &CharsConsumed)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y is not a valid job.\n"), &ArgV[2]);
                return EXIT_FAILURE;
            }
            JobId = (DWORD)llTemp;
            if (JobId == 0) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y is not a valid job.\n"), &ArgV[2]);
                return EXIT_FAILURE;
            }
            if (!YoriCallGetJobOutput(JobId, &Output, &Errors)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%i could not return errors.\n"), JobId);
                return EXIT_FAILURE;
            }
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &Errors);
            YoriCallFreeYoriString(&Errors);
            YoriCallFreeYoriString(&Output);
        } else if (YoriLibCompareStringLitIns(&ArgV[1], _T("exitcode")) == 0) {
            BOOL HasCompleted;
            BOOL HasOutput;
            DWORD ExitCode;
            YORI_STRING Command;
            HasCompleted = FALSE;
            HasOutput = FALSE;

            YoriLibInitEmptyString(&Command);

            if (ArgC < 3) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Job not specified\n"));
                return EXIT_FAILURE;
            }
            if (!YoriLibStringToNumber(&ArgV[2], TRUE, &llTemp, &CharsConsumed)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y is not a valid job.\n"), &ArgV[2]);
                return EXIT_FAILURE;
            }
            JobId = (DWORD)llTemp;
            if (JobId == 0) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y is not a valid job.\n"), &ArgV[2]);
                return EXIT_FAILURE;
            }
            if (!YoriCallGetJobInformation(JobId, &HasCompleted, &HasOutput, &ExitCode, &Command)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%i could not return exit code.\n"), JobId);
                return EXIT_FAILURE;
            }

            YoriCallFreeYoriString(&Command);

            if (!HasCompleted) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%i has not completed.\n"), JobId);
                return EXIT_FAILURE;
            }

            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%i"), ExitCode);
        } else if (YoriLibCompareStringLitIns(&ArgV[1], _T("kill")) == 0) {
            if (ArgC < 3) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Job not specified\n"));
                return EXIT_FAILURE;
            }
            if (!YoriLibStringToNumber(&ArgV[2], TRUE, &llTemp, &CharsConsumed)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y is not a valid job.\n"), &ArgV[2]);
                return EXIT_FAILURE;
            }
            JobId = (DWORD)llTemp;
            if (JobId == 0) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y is not a valid job.\n"), &ArgV[2]);
                return EXIT_FAILURE;
            }
            if (!YoriCallTerminateJob(JobId)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%i could not be terminated.\n"), JobId);
                return EXIT_FAILURE;
            }
        } else if (YoriLibCompareStringLitIns(&ArgV[1], _T("nice")) == 0) {
            if (ArgC < 3) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Job not specified\n"));
                return EXIT_FAILURE;
            }
            if (!YoriLibStringToNumber(&ArgV[2], TRUE, &llTemp, &CharsConsumed)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y is not a valid job.\n"), &ArgV[2]);
                return EXIT_FAILURE;
            }
            JobId = (DWORD)llTemp;
            if (JobId == 0) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y is not a valid job.\n"), &ArgV[2]);
                return EXIT_FAILURE;
            }
            if (!YoriCallSetJobPriority(JobId, IDLE_PRIORITY_CLASS)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%i could not have its priority changed.\n"), JobId);
                return EXIT_FAILURE;
            }
        } else if (YoriLibCompareStringLitIns(&ArgV[1], _T("output")) == 0) {
            YORI_STRING Output;
            YORI_STRING Errors;
            if (ArgC < 3) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Job not specified\n"));
                return EXIT_FAILURE;
            }
            if (!YoriLibStringToNumber(&ArgV[2], TRUE, &llTemp, &CharsConsumed)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y is not a valid job.\n"), &ArgV[2]);
                return EXIT_FAILURE;
            }
            JobId = (DWORD)llTemp;
            if (JobId == 0) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y is not a valid job.\n"), &ArgV[2]);
                return EXIT_FAILURE;
            }
            if (!YoriCallGetJobOutput(JobId, &Output, &Errors)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%i could not return output.\n"), JobId);
                return EXIT_FAILURE;
            }
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &Output);
            YoriCallFreeYoriString(&Output);
            YoriCallFreeYoriString(&Errors);
        }
    }
    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
