/**
 * @file scut/scut.c
 *
 * A command line tool to manipulate shortcuts
 *
 * Copyright (c) 2004-2018 Malcolm Smith
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
const GUID IID_IPersistFile = { 0x0000010BL, 0, 0, { 0xC0, 0, 0, 0, 0, 0, 0, 0x46 } };

/**
 The IPersistFile interface, composed of a pointer to a set of functions.
 */
typedef struct IPersistFile {

    /**
     The function pointer table associated with this object.
     */
     struct IPersistFileVtbl *Vtbl;
} IPersistFile;

/**
 Indicates the GUID of the class implementing the functionality.
 */
typedef HRESULT STDMETHODCALLTYPE IPersistFile_GetClassID (IPersistFile * This, GUID *pClassID);

/**
 Indicates whether the object has been modified since it was last written
 to disk.
 */
typedef HRESULT STDMETHODCALLTYPE IPersistFile_IsDirty (IPersistFile * This);

/**
 Load the object from disk.
 */
typedef HRESULT STDMETHODCALLTYPE IPersistFile_Load (IPersistFile * This, LPCWSTR pszFileName, DWORD dwMode);

/**
 Save the object to disk.
 */
typedef HRESULT STDMETHODCALLTYPE IPersistFile_Save (IPersistFile * This, LPCWSTR pszFileName, BOOL fRemember);

/**
 Indicate that a save has completed and the object can be modified again.
 */
typedef HRESULT STDMETHODCALLTYPE IPersistFile_SaveCompleted (IPersistFile * This, LPCWSTR pszFileName);

/**
 Get the current file name associated with the object.
 */
typedef HRESULT STDMETHODCALLTYPE IPersistFile_GetCurFile (IPersistFile * This, LPCWSTR *ppszFileName);
    

/**
 A set of functions defined by the IPersistFile interface.
 */
typedef struct IPersistFileVtbl {

    /**
     Standard COM QueryInterface method.
     */
    IUnknown_QueryInterface * QueryInterface;

    /**
     Standard COM AddRef method.
     */
    IUnknown_AddRef * AddRef;

    /**
     Standard COM Release method.
     */
    IUnknown_Release * Release;
    
    /**
     Indicates the GUID of the class implementing the functionality.
     */
    IPersistFile_GetClassID * GetClassID;

    /**
     Indicates whether the object has been modified since it was last written
     to disk.
     */
    IPersistFile_IsDirty * IsDirty;

    /**
     Load the object from disk.
     */
    IPersistFile_Load * Load;

    /**
     Save the object to disk.
     */
    IPersistFile_Save * Save;

    /**
     Indicate that a save has completed and the object can be modified again.
     */
    IPersistFile_SaveCompleted * SaveCompleted;

    /**
     Get the current file name associated with the object.
     */
    IPersistFile_GetCurFile * GetCurFile;
    
} IPersistFileVtbl;

/**
 The ShellLink interface.
 */
const GUID CLSID_ShellLink = { 0x00021401L, 0, 0, { 0xC0, 0, 0, 0, 0, 0, 0, 0x46 } };

/**
 The IShellLink interface.
 */
const GUID IID_IShellLinkW = { 0x000214F9L, 0, 0, { 0xC0, 0, 0, 0, 0, 0, 0, 0x46 } };

/**
 An instance of the IShellLink interface, consisting only of function pointers.
 */
typedef struct IShellLinkW {

    /**
     The function pointer table associated with this object.
     */
    struct IShellLinkWVtbl * Vtbl;
} IShellLinkW;

typedef struct IShellLinkWVtbl IShellLinkWVtbl;

/**
 Get the path to the target on a shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_GetPath (IShellLinkW * This, LPWSTR pszFile, int cchMaxPath, WIN32_FIND_DATAW *pfd, DWORD fFlags);

/**
 Get a PIDL associated with a shortcut.  Used for shortcuts to objects
 other than files, and not used by this application.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_GetIDList (IShellLinkW * This, PVOID ppidl);

/**
 Set a PIDL to associate with a shortcut.  Used for shortcuts to objects
 other than files, and not used by this application.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_SetIDList (IShellLinkW * This, PVOID pidl);

/**
 Get the description associated with the shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_GetDescription (IShellLinkW * This, LPWSTR pszName, int cchMaxName);

/**
 Set a description to be associated with the shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_SetDescription (IShellLinkW * This, LPCWSTR pszName);

/**
 Get the directory to make current when launching from the shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_GetWorkingDirectory (IShellLinkW * This, LPWSTR pszDir, int cchMaxPath);

/**
 Set the directory to make current when launching from the shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_SetWorkingDirectory (IShellLinkW * This, LPCWSTR pszDir);

/**
 Get the arguments (parameters) to pass to the object pointed to by the shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_GetArguments (IShellLinkW * This, LPWSTR pszArgs, int cchMaxPath);

/**
 Set the arguments (parameters) to pass to the object pointed to by the shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_SetArguments (IShellLinkW * This, LPCWSTR pszArgs);

/**
 Get the hotkey associated with the shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_GetHotkey (IShellLinkW * This, WORD *pwHotkey);

/**
 Set the hotkey associated with the shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_SetHotkey (IShellLinkW * This, WORD wHotkey);

/**
 Get the window display state to use when launching from the shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_GetShowCmd (IShellLinkW * This, int *piShowCmd);

/**
 Set the window display state to use when launching from the shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_SetShowCmd (IShellLinkW * This, int iShowCmd);

/**
 Get the location of the icon to use in the shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_GetIconLocation (IShellLinkW * This, LPWSTR pszIconPath, int cchIconPath, int *piIcon);

/**
 Set the location of the icon to use in the shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_SetIconLocation (IShellLinkW * This, LPCWSTR pszIconPath, int iIcon);

/**
 Set a relative path on a shortcut.  Not used by this application.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_SetRelativePath (IShellLinkW * This, LPCWSTR pszPathRel, DWORD dwReserved);

/**
 Resolve a shell link (useful if the target has moved.)  Not used by this application.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_Resolve (IShellLinkW * This, HWND hwnd, DWORD fFlags);

/**
 Set the path to the target in a shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_SetPath (IShellLinkW * This, LPCWSTR pszFile);

/**
 A set of functions defined by the IShellLink interface.
 */
struct IShellLinkWVtbl {

    /**
     Standard COM QueryInterface method.
     */
    IUnknown_QueryInterface * QueryInterface;

    /**
     Standard COM AddRef method.
     */
    IUnknown_AddRef * AddRef;

    /**
     Standard COM Release method.
     */
    IUnknown_Release * Release;
    

    /**
     Get the path to the target on a shortcut.
     */
    IShellLink_GetPath * GetPath;

    /**
     Get a PIDL associated with a shortcut.  Used for shortcuts to objects
     other than files, and not used by this application.
     */
    IShellLink_GetIDList * GetIDList;

    /**
     Set a PIDL to associate with a shortcut.  Used for shortcuts to objects
     other than files, and not used by this application.
     */
    IShellLink_SetIDList * SetIDList;

    /**
     Get the description associated with the shortcut.
     */
    IShellLink_GetDescription * GetDescription;

    /**
     Set a description to be associated with the shortcut.
     */
    IShellLink_SetDescription * SetDescription;

    /**
     Get the directory to make current when launching from the shortcut.
     */
    IShellLink_GetWorkingDirectory * GetWorkingDirectory;

    /**
     Set the directory to make current when launching from the shortcut.
     */
    IShellLink_SetWorkingDirectory * SetWorkingDirectory;

    /**
     Get the arguments (parameters) to pass to the object pointed to by the shortcut.
     */
    IShellLink_GetArguments * GetArguments;

    /**
     Set the arguments (parameters) to pass to the object pointed to by the shortcut.
     */
    IShellLink_SetArguments * SetArguments;

    /**
     Get the hotkey associated with the shortcut.
     */
    IShellLink_GetHotkey * GetHotkey;

    /**
     Set the hotkey associated with the shortcut.
     */
    IShellLink_SetHotkey * SetHotkey;

    /**
     Get the window display state to use when launching from the shortcut.
     */
    IShellLink_GetShowCmd * GetShowCmd;

    /**
     Set the window display state to use when launching from the shortcut.
     */
    IShellLink_SetShowCmd * SetShowCmd;

    /**
     Get the location of the icon to use in the shortcut.
     */
    IShellLink_GetIconLocation * GetIconLocation;

    /**
     Set the location of the icon to use in the shortcut.
     */
    IShellLink_SetIconLocation * SetIconLocation;

    /**
     Set a relative path on a shortcut.  Not used by this application.
     */
    IShellLink_SetRelativePath * SetRelativePath;

    /**
     Resolve a shell link (useful if the target has moved.)  Not used by this application.
     */
    IShellLink_Resolve * Resolve;

    /**
     Set the path to the target in a shortcut.
     */
    IShellLink_SetPath * SetPath;
};


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
        "     [-desc description] [-hotkey hotkey]\n"
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
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Scut %i.%i\n"), SCUT_VER_MAJOR, SCUT_VER_MINOR);
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

/**
 The main entrypoint for scut.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return ExitCode for the process.
 */
DWORD
ymain(
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
                YoriLibDisplayMitLicense(_T("2004-2018"));
                ExitCode = EXIT_SUCCESS;
                goto Exit;
            } else if (i + 1 >= ArgC) {
                //
                //  Every parameter needs an argument
                //
                ScutHelp();
                goto Exit;
            }
            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("create")) == 0) {
                op = ScutOperationCreate;
                YoriLibFreeStringContents(&szFile);
                YoriLibUserStringToSingleFilePath(&ArgV[i + 1], FALSE, &szFile);
                ArgumentUnderstood = TRUE;
            }
            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("modify")) == 0) {
                op = ScutOperationModify;
                YoriLibFreeStringContents(&szFile);
                YoriLibUserStringToSingleFilePath(&ArgV[i + 1], FALSE, &szFile);
                ArgumentUnderstood = TRUE;
            }
            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("exec")) == 0) {
                op = ScutOperationExec;
                YoriLibFreeStringContents(&szFile);
                YoriLibUserStringToSingleFilePath(&ArgV[i + 1], FALSE, &szFile);
                ArgumentUnderstood = TRUE;
            }
            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("dump")) == 0) {
                op = ScutOperationDump;
                YoriLibFreeStringContents(&szFile);
                YoriLibUserStringToSingleFilePath(&ArgV[i + 1], FALSE, &szFile);
                ArgumentUnderstood = TRUE;
            }
    
            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("args")) == 0) {
                szArgs = ArgV[i + 1].StartOfString;
                ArgumentUnderstood = TRUE;
            }
            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("desc")) == 0) {
                szDesc = ArgV[i + 1].StartOfString;
                ArgumentUnderstood = TRUE;
            }
            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("f")) == 0) {
                YsFormatString.StartOfString = ArgV[i + 1].StartOfString;
                YsFormatString.LengthInChars = ArgV[i + 1].LengthInChars;
                YsFormatString.LengthAllocated = ArgV[i + 1].LengthAllocated;
                ArgumentUnderstood = TRUE;
            }
            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("hotkey")) == 0) {
                llTemp = 0;
                YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed);
                wHotkey = (WORD)llTemp;
                ArgumentUnderstood = TRUE;
            }
            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("iconpath")) == 0) {
                YoriLibFreeStringContents(&szIcon);
                YoriLibUserStringToSingleFilePath(&ArgV[i + 1], FALSE, &szIcon);
                ArgumentUnderstood = TRUE;
            }
            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("iconindex")) == 0) {
                llTemp = 0;
                YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed);
                wIcon = (WORD)llTemp;
                ArgumentUnderstood = TRUE;
            }
            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("show")) == 0) {
                llTemp = 0;
                YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed);
                wShow = (WORD)llTemp;
                ArgumentUnderstood = TRUE;
            }
            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("target")) == 0) {
                szTarget = ArgV[i + 1].StartOfString;
                ArgumentUnderstood = TRUE;
            }
            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("workingdir")) == 0) {
                YoriLibFreeStringContents(&szWorkingDir);
                YoriLibUserStringToSingleFilePath(&ArgV[i + 1], FALSE, &szWorkingDir);
                ArgumentUnderstood = TRUE;
            }
            i++;
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
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("CoInitialize failure: %x\n"), (int)hRes);
        goto Exit;
    }

    hRes = DllOle32.pCoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, (void **)&scut);

    if (!SUCCEEDED(hRes)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("CoCreateInstance failure: %x\n"), (int)hRes);
        goto Exit;
    }

    hRes = scut->Vtbl->QueryInterface(scut, &IID_IPersistFile, (void **)&savedfile);
    if (!SUCCEEDED(hRes)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("QueryInstance IPersistFile failure: %x\n"), (int)hRes);
        goto Exit;
    }

    if (op == ScutOperationModify ||
        op == ScutOperationExec ||
        op == ScutOperationDump) {
        hRes = savedfile->Vtbl->Load(savedfile, szFile.StartOfString, TRUE);
        if (!SUCCEEDED(hRes)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Load failure: %x\n"), (int)hRes);
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

    if (op == ScutOperationModify ||
        op == ScutOperationCreate) {
        hRes = savedfile->Vtbl->Save(savedfile, szFile.StartOfString, TRUE);
        if (!SUCCEEDED(hRes)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Save failure: %x\n"), (int)hRes);
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
