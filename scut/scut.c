/**
 * @file scut/scut.c
 *
 * A command line tool to manipulate shortcuts
 *
 * Copyright (c) 2004-2019 Malcolm Smith
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
 A declaration for a GUID defining the shell file API interface.
 */
extern const GUID IID_IPersistFile;

/**
 The ShellLink interface.
 */
extern const GUID CLSID_ShellLink;

/**
 The IShellLink interface.
 */
extern const GUID IID_IShellLinkW;

/**
 The IShellLinkDataList interface.
 */
extern const GUID IID_IShellLinkDataList;

/**
 A list of operations supported by scut.
 */
typedef enum ScutOperations {
    ScutOperationUnknown=0,
    ScutOperationCreate,
    ScutOperationModify,
    ScutOperationExec,
    ScutOperationDump
} ScutOperations;

/**
 Help text for this application.
 */
const
CHAR strScutHelpText[] =
        "\n"
        "Create, modify, display or execute Window shortcuts.\n"
        "\n"
        "SCUT -license\n"
        "SCUT -create|-modify <filename> [-target target] [-args args]\n"
        "     [-desc description] [-deleteconsolesettings] [-hotkey hotkey]\n"
        "     [-iconpath filename [-iconindex index]] [-show showcmd]\n"
        "     [-workingdir workingdir]\n"
        "SCUT -exec <filename> [-target target] [-args args] [-show showcmd]\n"
        "     [-workingdir workingdir]\n"
        "SCUT [-f fmt] -dump <filename>\n";

/**
 Display help text and license for the scut application.
 */
VOID
ScutHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Scut %i.%02i\n"), SCUT_VER_MAJOR, SCUT_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strScutHelpText);
}

/**
 A callback function to expand any known variables found when parsing the
 format string.

 @param OutputString A pointer to the output string to populate with data
        if a known variable is found.  The allocated length indicates the
        amount of the string that can be written to.

 @param VariableName The variable name to expand.

 @param Context Pointer to an IShellLinkW interface that can supply the data
        to populate.
 
 @return The number of characters successfully populated, or the number of
         characters required in order to successfully populate, or zero
         on error.
 */
DWORD
ScutExpandVariables(
    __inout PYORI_STRING OutputString,
    __in PYORI_STRING VariableName,
    __in PVOID Context
    )
{
    TCHAR szTemp[MAX_PATH];
    DWORD CharsNeeded;
    int   wTemp = 0;
    BOOL  Numeric = FALSE;
    IShellLinkW * scut = (IShellLinkW *)Context;

    if (YoriLibCompareStringWithLiteral(VariableName, _T("TARGET")) == 0) {
        if (scut->Vtbl->GetPath(scut, szTemp, MAX_PATH, NULL, 0) != NOERROR) {
            return 0;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("ARGS")) == 0) {
        if (scut->Vtbl->GetArguments(scut, szTemp, MAX_PATH) != NOERROR) {
            return 0;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("WORKINGDIR")) == 0) {
        if (scut->Vtbl->GetWorkingDirectory(scut, szTemp, MAX_PATH) != NOERROR) {
            return 0;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("DESCRIPTION")) == 0) {
        if (scut->Vtbl->GetDescription(scut, szTemp, MAX_PATH) != NOERROR) {
            return 0;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("ICONPATH")) == 0) {
        if (scut->Vtbl->GetIconLocation(scut, szTemp, MAX_PATH, &wTemp) != NOERROR) {
            return 0;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("ICONINDEX")) == 0) {
        if (scut->Vtbl->GetIconLocation(scut, szTemp, MAX_PATH, &wTemp) != NOERROR) {
            return 0;
        }
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("SHOW")) == 0) {
        if (scut->Vtbl->GetShowCmd(scut, &wTemp) != NOERROR) {
            return 0;
        }
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("HOTKEY")) == 0) {
        USHORT ShortTemp;
        if (scut->Vtbl->GetHotkey(scut, &ShortTemp) != NOERROR) {
            return 0;
        }
        wTemp = ShortTemp;
        Numeric = TRUE;
    } else {
        return 0;
    }

    if (Numeric) {
        CharsNeeded = YoriLibSPrintf(szTemp, _T("%i"), wTemp);
    } else {
        CharsNeeded = _tcslen(szTemp);
    }
    if (OutputString->LengthAllocated < CharsNeeded) {
        return CharsNeeded;
    }

    memcpy(OutputString->StartOfString, szTemp, CharsNeeded * sizeof(TCHAR));
    OutputString->LengthInChars = CharsNeeded;
    return CharsNeeded;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the scut builtin command.
 */
#define ENTRYPOINT YoriCmd_SCUT
#else
/**
 The main entrypoint for the scut standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for scut.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return ExitCode for the process.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    ScutOperations op     = ScutOperationUnknown;
    YORI_STRING szFile;

    TCHAR * szArgs        = NULL;
    TCHAR * szDesc        = NULL;
    WORD    wHotkey       = (WORD)-1;
    YORI_STRING szIcon;
    WORD    wIcon         = 0;
    WORD    wShow         = (WORD)-1;
    TCHAR * szTarget      = NULL;    
    YORI_STRING szWorkingDir;
    IShellLinkW *scut     = NULL;
    IPersistFile *savedfile = NULL;
    IShellLinkDataList *ShortcutDataList = NULL;
    YORI_STRING Arg;
    LONGLONG llTemp;
    DWORD   CharsConsumed;
    DWORD   ExitCode;
    LPTSTR FormatString = _T("Target:          $TARGET$\n")
                          _T("Arguments:       $ARGS$\n")
                          _T("Working dir:     $WORKINGDIR$\n")
                          _T("Description:     $DESCRIPTION$\n")
                          _T("Icon Path:       $ICONPATH$\n")
                          _T("Icon Index:      $ICONINDEX$\n")
                          _T("Show State:      $SHOW$\n")
                          _T("Hotkey:          $HOTKEY$\n");

    HRESULT hRes;
    BOOL    ArgumentUnderstood;
    BOOL    DeleteConsoleSettings = FALSE;
    DWORD   i;
    YORI_STRING YsFormatString;

    YoriLibInitEmptyString(&szFile);
    YoriLibInitEmptyString(&szIcon);
    YoriLibInitEmptyString(&szWorkingDir);
    YoriLibInitEmptyString(&YsFormatString);

    ExitCode = EXIT_FAILURE;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                ScutHelp();
                ExitCode = EXIT_SUCCESS;
                goto Exit;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2004-2019"));
                ExitCode = EXIT_SUCCESS;
                goto Exit;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("create")) == 0) {
                if (i + 1 < ArgC) {
                    op = ScutOperationCreate;
                    YoriLibFreeStringContents(&szFile);
                    YoriLibUserStringToSingleFilePath(&ArgV[i + 1], FALSE, &szFile);
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("modify")) == 0) {
                if (i + 1 < ArgC) {
                    op = ScutOperationModify;
                    YoriLibFreeStringContents(&szFile);
                    YoriLibUserStringToSingleFilePath(&ArgV[i + 1], FALSE, &szFile);
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("exec")) == 0) {
                if (i + 1 < ArgC) {
                    op = ScutOperationExec;
                    YoriLibFreeStringContents(&szFile);
                    YoriLibUserStringToSingleFilePath(&ArgV[i + 1], FALSE, &szFile);
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("dump")) == 0) {
                if (i + 1 < ArgC) {
                    op = ScutOperationDump;
                    YoriLibFreeStringContents(&szFile);
                    YoriLibUserStringToSingleFilePath(&ArgV[i + 1], FALSE, &szFile);
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("args")) == 0) {
                if (i + 1 < ArgC) {
                    szArgs = ArgV[i + 1].StartOfString;
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("desc")) == 0) {
                if (i + 1 < ArgC) {
                    szDesc = ArgV[i + 1].StartOfString;
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("deleteconsolesettings")) == 0) {
                if (op == ScutOperationModify || op == ScutOperationCreate) {
                    DeleteConsoleSettings = TRUE;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("f")) == 0) {
                if (i + 1 < ArgC) {
                    YsFormatString.StartOfString = ArgV[i + 1].StartOfString;
                    YsFormatString.LengthInChars = ArgV[i + 1].LengthInChars;
                    YsFormatString.LengthAllocated = ArgV[i + 1].LengthAllocated;
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("hotkey")) == 0) {
                if (i + 1 < ArgC) {
                    llTemp = 0;
                    YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed);
                    wHotkey = (WORD)llTemp;
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("iconpath")) == 0) {
                if (i + 1 < ArgC) {
                    YoriLibFreeStringContents(&szIcon);
                    YoriLibUserStringToSingleFilePath(&ArgV[i + 1], FALSE, &szIcon);
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("iconindex")) == 0) {
                if (i + 1 < ArgC) {
                    llTemp = 0;
                    YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed);
                    wIcon = (WORD)llTemp;
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("show")) == 0) {
                if (i + 1 < ArgC) {
                    llTemp = 0;
                    YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed);
                    wShow = (WORD)llTemp;
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("target")) == 0) {
                if (i + 1 < ArgC) {
                    szTarget = ArgV[i + 1].StartOfString;
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("workingdir")) == 0) {
                if (i + 1 < ArgC) {
                    YoriLibFreeStringContents(&szWorkingDir);
                    YoriLibUserStringToSingleFilePath(&ArgV[i + 1], FALSE, &szWorkingDir);
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            }
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (op == ScutOperationUnknown) {
        ScutHelp();
        goto Exit;
    }

    if (op == ScutOperationDump && YsFormatString.StartOfString == NULL) {
        YoriLibConstantString(&YsFormatString, FormatString);
    }

    YoriLibLoadOle32Functions();
    if (DllOle32.pCoCreateInstance == NULL || DllOle32.pCoInitialize == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("COM not found\n"));
        goto Exit;
    }

    hRes = DllOle32.pCoInitialize(NULL);
    if (!SUCCEEDED(hRes)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("CoInitialize failure: %x\n"), hRes);
        goto Exit;
    }

    hRes = DllOle32.pCoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, (void **)&scut);

    if (!SUCCEEDED(hRes)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("CoCreateInstance failure: %x\n"), hRes);
        goto Exit;
    }

    hRes = scut->Vtbl->QueryInterface(scut, &IID_IPersistFile, (void **)&savedfile);
    if (!SUCCEEDED(hRes)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("QueryInstance IPersistFile failure: %x\n"), hRes);
        goto Exit;
    }

    hRes = scut->Vtbl->QueryInterface(scut, &IID_IShellLinkDataList, (void **)&ShortcutDataList);
    if (!SUCCEEDED(hRes)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("QueryInstance IShellLinkDataList failure: %x\n"), hRes);
        goto Exit;
    }

    if (op == ScutOperationModify ||
        op == ScutOperationExec ||
        op == ScutOperationDump) {

        DWORD OpenMode;

        //
        //  Open for read only unless we intend to modify
        //

        OpenMode = 0;
        if (op == ScutOperationModify) {
            OpenMode = 1;
        }

        hRes = savedfile->Vtbl->Load(savedfile, szFile.StartOfString, OpenMode);
        if (!SUCCEEDED(hRes)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Load failure: %x\n"), hRes);
            goto Exit;
        }
    }

    if (op == ScutOperationDump) {
        YORI_STRING DisplayString;
        YoriLibInitEmptyString(&DisplayString);
        YoriLibExpandCommandVariables(&YsFormatString, '$', FALSE, ScutExpandVariables, scut, &DisplayString);
        if (DisplayString.StartOfString != NULL) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
            YoriLibFreeStringContents(&DisplayString);
            ExitCode = EXIT_SUCCESS;
        }
        goto Exit;
    }

    if (szTarget) {
        if (scut->Vtbl->SetPath(scut, szTarget) != NOERROR) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("SetPath failure\n"));
            goto Exit;
        }
    }

    if (szArgs) {
        if (scut->Vtbl->SetArguments(scut, szArgs) != NOERROR) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("SetArguments failure\n"));
            goto Exit;
        }
    }

    if (szDesc) {
        if (scut->Vtbl->SetDescription(scut, szDesc) != NOERROR) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("SetDescription failure\n"));
            goto Exit;
        }
    }

    if (wHotkey != (WORD)-1) {
        if (scut->Vtbl->SetHotkey(scut, wHotkey) != NOERROR) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("SetHotkey failure\n"));
            goto Exit;
        }
    }

    if (szIcon.StartOfString) {
        if (scut->Vtbl->SetIconLocation(scut, szIcon.StartOfString, wIcon) != NOERROR) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("SetIconLocation failure\n"));
            goto Exit;
        }
    }

    if (wShow != (WORD)-1) {
        if (scut->Vtbl->SetShowCmd(scut, wShow) != NOERROR) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("SetShowCmd failure\n"));
            goto Exit;
        }
    }

    if (szWorkingDir.StartOfString) {
        if (scut->Vtbl->SetWorkingDirectory(scut, szWorkingDir.StartOfString) != NOERROR) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("SetWorkingDirectory failure\n"));
            goto Exit;
        }
    }

    if (op == ScutOperationModify && DeleteConsoleSettings) {
        hRes = ShortcutDataList->Vtbl->RemoveDataBlock(ShortcutDataList, ISHELLLINKDATALIST_CONSOLE_PROPS_SIG);
        if (!SUCCEEDED(hRes)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("RemoveDataBlock failure: %x\n"), hRes);
            goto Exit;
        }
    }

    if (op == ScutOperationModify ||
        op == ScutOperationCreate) {
        hRes = savedfile->Vtbl->Save(savedfile, szFile.StartOfString, TRUE);
        if (!SUCCEEDED(hRes)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Save failure: %x\n"), hRes);
            goto Exit;
        }
    } else if (op == ScutOperationExec) {
        TCHAR szFileBuf[MAX_PATH];
        TCHAR szArgsBuf[MAX_PATH];
        TCHAR szDir[MAX_PATH];
        INT nShow;
        HINSTANCE hApp;
        if (scut->Vtbl->GetWorkingDirectory(scut, szDir, MAX_PATH) != NOERROR) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("GetWorkingDirectory failure\n"));
            goto Exit;
        }
        if (scut->Vtbl->GetArguments(scut, szArgsBuf,MAX_PATH) != NOERROR) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("GetArguments failure\n"));
            goto Exit;
        }
        if (scut->Vtbl->GetPath(scut, szFileBuf, MAX_PATH, NULL, 0) != NOERROR) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("GetPath failure\n"));
            goto Exit;
        }
        if (scut->Vtbl->GetShowCmd(scut, &nShow) != NOERROR) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("GetShowCmd failure\n"));
            goto Exit;
        }

        YoriLibLoadShell32Functions();

        if (DllShell32.pShellExecuteW == NULL) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ShellExecuteW export not found\n"));
            goto Exit;
        }

        hApp = DllShell32.pShellExecuteW(NULL, NULL, szFileBuf, szArgsBuf, szDir, nShow);
        if ((ULONG_PTR)hApp <= 32) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ShellExecute failure: %i\n"), (int)(ULONG_PTR)hApp);
            goto Exit;
        }
    }

    ExitCode = EXIT_SUCCESS;

Exit:

    YoriLibFreeStringContents(&szFile);
    YoriLibFreeStringContents(&szIcon);
    YoriLibFreeStringContents(&szWorkingDir);

    return ExitCode;
}

// vim:sw=4:ts=4:et:
