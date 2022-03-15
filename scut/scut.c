/**
 * @file scut/scut.c
 *
 * A command line tool to manipulate shortcuts
 *
 * Copyright (c) 2004-2022 Malcolm Smith
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
ScutHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Scut %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strScutHelpText);
}

/**
 Context passed to each variable expansion.
 */
typedef struct _SCUT_EXPAND_CONTEXT {

    /**
     Pointer to the shortcut.
     */
    IShellLinkW *ShellLink;

    /**
     Optionally points to extra console properties.  May be NULL if the
     shortcut doesn't specify console properties.
     */
    PISHELLLINKDATALIST_CONSOLE_PROPS ConsoleProperties;
} SCUT_EXPAND_CONTEXT, *PSCUT_EXPAND_CONTEXT;

/**
 Invert the byte order of a color so it will be displayed in RGB order.
 */
#define SCUT_INVERT_COLOR_BYTES(C) \
    (((C) & 0xFF0000) >> 16) |     \
    (((C) & 0x00FF00))       |     \
    (((C) & 0x0000FF) << 16)

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
    TCHAR szDisplay[MAX_PATH];
    DWORD CharsNeeded;
    DWORD dwDisplay = 0;
    int iTemp;
    BOOL  Numeric = FALSE;
    DWORD HexDigits = 0;
    PSCUT_EXPAND_CONTEXT ExpandContext;
    IShellLinkW * scut;
    PISHELLLINKDATALIST_CONSOLE_PROPS ConsoleProps;

    ExpandContext = (PSCUT_EXPAND_CONTEXT)Context;
    scut = ExpandContext->ShellLink;
    ConsoleProps = ExpandContext->ConsoleProperties;

    if (YoriLibCompareStringWithLiteral(VariableName, _T("TARGET")) == 0) {
        if (scut->Vtbl->GetPath(scut, szDisplay, MAX_PATH, NULL, 0) != NOERROR) {
            return 0;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("ARGS")) == 0) {
        if (scut->Vtbl->GetArguments(scut, szDisplay, MAX_PATH) != NOERROR) {
            return 0;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("WORKINGDIR")) == 0) {
        if (scut->Vtbl->GetWorkingDirectory(scut, szDisplay, MAX_PATH) != NOERROR) {
            return 0;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("DESCRIPTION")) == 0) {
        if (scut->Vtbl->GetDescription(scut, szDisplay, MAX_PATH) != NOERROR) {
            return 0;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("ICONPATH")) == 0) {

        if (scut->Vtbl->GetIconLocation(scut, szDisplay, MAX_PATH, &iTemp) != NOERROR) {
            return 0;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("ICONINDEX")) == 0) {
        if (scut->Vtbl->GetIconLocation(scut, szDisplay, MAX_PATH, &iTemp) != NOERROR) {
            return 0;
        }
        dwDisplay = iTemp;
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("SHOW")) == 0) {
        if (scut->Vtbl->GetShowCmd(scut, &iTemp) != NOERROR) {
            return 0;
        }
        dwDisplay = iTemp;
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("HOTKEY")) == 0) {
        USHORT ShortTemp;
        if (scut->Vtbl->GetHotkey(scut, &ShortTemp) != NOERROR) {
            return 0;
        }
        dwDisplay = ShortTemp;
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("WINDOWCOLOR")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->WindowColor;
        HexDigits = 2;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("POPUPCOLOR")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->PopupColor;
        HexDigits = 2;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("SCREENBUFFERSIZE_X")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->ScreenBufferSize.X;
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("SCREENBUFFERSIZE_Y")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->ScreenBufferSize.Y;
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("WINDOWSIZE_X")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->WindowSize.X;
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("WINDOWSIZE_Y")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->WindowSize.Y;
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("WINDOWPOSITION_X")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->WindowPosition.X;
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("WINDOWPOSITION_Y")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->WindowPosition.Y;
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("FONTNUMBER")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->FontNumber;
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("INPUTBUFFERSIZE")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->InputBufferSize;
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("FONTSIZE_X")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->FontSize.X;
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("FONTSIZE_Y")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->FontSize.Y;
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("FONTFAMILY")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->FontFamily;
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("FONTWEIGHT")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->FontWeight;
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("FONT")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        YoriLibSPrintfS(szDisplay, sizeof(szDisplay)/sizeof(szDisplay[0]), _T("%s"), ConsoleProps->FaceName);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("CURSORSIZE")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->CursorSize;
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("FULLSCREEN")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->FullScreen;
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("QUICKEDIT")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->QuickEdit;
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("INSERT")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->InsertMode;
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("AUTOPOSITION")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->AutoPosition;
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("HISTORYBUFFERSIZE")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->HistoryBufferSize;
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("HISTORYBUFFERCOUNT")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->NumberOfHistoryBuffers;
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("HISTORYBUFFERCOUNT")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->NumberOfHistoryBuffers;
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("NOHISTORYDUPLICATES")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->RemoveHistoryDuplicates;
        Numeric = TRUE;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("COLOR_BLACK")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[0]);
        HexDigits = 6;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("COLOR_BLUE")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_BLUE]);
        HexDigits = 6;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("COLOR_GREEN")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_GREEN]);
        HexDigits = 6;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("COLOR_CYAN")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_BLUE|FOREGROUND_GREEN]);
        HexDigits = 6;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("COLOR_RED")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_RED]);
        HexDigits = 6;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("COLOR_MAGENTA")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_RED|FOREGROUND_BLUE]);
        HexDigits = 6;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("COLOR_BROWN")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_RED|FOREGROUND_GREEN]);
        HexDigits = 6;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("COLOR_GRAY")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE]);
        HexDigits = 6;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("COLOR_DARKGRAY")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_INTENSITY]);
        HexDigits = 6;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("COLOR_LIGHTBLUE")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_INTENSITY|FOREGROUND_BLUE]);
        HexDigits = 6;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("COLOR_LIGHTGREEN")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_INTENSITY|FOREGROUND_GREEN]);
        HexDigits = 6;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("COLOR_LIGHTCYAN")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_INTENSITY|FOREGROUND_GREEN|FOREGROUND_BLUE]);
        HexDigits = 6;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("COLOR_LIGHTRED")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_INTENSITY|FOREGROUND_RED]);
        HexDigits = 6;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("COLOR_LIGHTMAGENTA")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_INTENSITY|FOREGROUND_RED|FOREGROUND_BLUE]);
        HexDigits = 6;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("COLOR_YELLOW")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_INTENSITY|FOREGROUND_RED|FOREGROUND_GREEN]);
        HexDigits = 6;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("COLOR_WHITE")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_INTENSITY|FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE]);
        HexDigits = 6;
    } else {
        return 0;
    }

    if (HexDigits == 6) {
        CharsNeeded = YoriLibSPrintf(szDisplay, _T("%06x"), dwDisplay);
    } else if (HexDigits == 2) {
        CharsNeeded = YoriLibSPrintf(szDisplay, _T("%02x"), dwDisplay);
    } else if (Numeric) {
        CharsNeeded = YoriLibSPrintf(szDisplay, _T("%i"), dwDisplay);
    } else {
        CharsNeeded = _tcslen(szDisplay);
    }
    if (OutputString->LengthAllocated < CharsNeeded) {
        return CharsNeeded;
    }

    memcpy(OutputString->StartOfString, szDisplay, CharsNeeded * sizeof(TCHAR));
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
    LPTSTR DefaultFormatString = _T("Target:                $TARGET$\n")
                                 _T("Arguments:             $ARGS$\n")
                                 _T("Working dir:           $WORKINGDIR$\n")
                                 _T("Description:           $DESCRIPTION$\n")
                                 _T("Icon Path:             $ICONPATH$\n")
                                 _T("Icon Index:            $ICONINDEX$\n")
                                 _T("Show State:            $SHOW$\n")
                                 _T("Hotkey:                $HOTKEY$\n");

    LPTSTR ConsoleFormatString = _T("Window Color:          $WINDOWCOLOR$\n")
                                 _T("Popup Color:           $POPUPCOLOR$\n")
                                 _T("Buffer Size:           $SCREENBUFFERSIZE_X$x$SCREENBUFFERSIZE_Y$\n")
                                 _T("Window Size:           $WINDOWSIZE_X$x$WINDOWSIZE_Y$\n")
                                 _T("Window Position:       $WINDOWPOSITION_X$x$WINDOWPOSITION_Y$\n")
                                 _T("Font Number:           $FONTNUMBER$\n")
                                 _T("Input Buffer Size:     $INPUTBUFFERSIZE$\n")
                                 _T("Font Size:             $FONTSIZE_X$x$FONTSIZE_Y$\n")
                                 _T("Font Family:           $FONTFAMILY$\n")
                                 _T("Font Weight:           $FONTWEIGHT$\n")
                                 _T("Font:                  $FONT$\n")
                                 _T("Cursor Size:           $CURSORSIZE$\n")
                                 _T("Full Screen:           $FULLSCREEN$\n")
                                 _T("QuickEdit:             $QUICKEDIT$\n")
                                 _T("Insert:                $INSERT$\n")
                                 _T("Auto Position:         $AUTOPOSITION$\n")
                                 _T("History Buffer Size:   $HISTORYBUFFERSIZE$\n")
                                 _T("History Buffer Count:  $HISTORYBUFFERCOUNT$\n")
                                 _T("No History Duplicates: $NOHISTORYDUPLICATES$\n");

    LPTSTR ConsoleFormatString2= _T("Window Color:          $WINDOWCOLOR$\n")
                                 _T("Color Black:           $COLOR_BLACK$\n")
                                 _T("Color Blue:            $COLOR_BLUE$\n")
                                 _T("Color Green:           $COLOR_GREEN$\n")
                                 _T("Color Cyan:            $COLOR_CYAN$\n")
                                 _T("Color Red:             $COLOR_RED$\n")
                                 _T("Color Magenta:         $COLOR_MAGENTA$\n")
                                 _T("Color Brown:           $COLOR_BROWN$\n")
                                 _T("Color Gray:            $COLOR_GRAY$\n")
                                 _T("Color Dark Gray:       $COLOR_DARKGRAY$\n")
                                 _T("Color Light Blue:      $COLOR_LIGHTBLUE$\n")
                                 _T("Color Light Green:     $COLOR_LIGHTGREEN$\n")
                                 _T("Color Light Cyan:      $COLOR_LIGHTCYAN$\n")
                                 _T("Color Light Red:       $COLOR_LIGHTRED$\n")
                                 _T("Color Light Magenta:   $COLOR_LIGHTMAGENTA$\n")
                                 _T("Color Yellow:          $COLOR_YELLOW$\n")
                                 _T("Color White:           $COLOR_WHITE$\n");

    HRESULT hRes;
    BOOL    ArgumentUnderstood;
    BOOL    DeleteConsoleSettings = FALSE;
    DWORD   i;
    YORI_STRING FormatString;
    PISHELLLINKDATALIST_CONSOLE_PROPS ConsoleProps = NULL;

    YoriLibInitEmptyString(&szFile);
    YoriLibInitEmptyString(&szIcon);
    YoriLibInitEmptyString(&szWorkingDir);
    YoriLibInitEmptyString(&FormatString);

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
                YoriLibDisplayMitLicense(_T("2004-2022"));
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
                    if (!YoriLibUserStringToSingleFilePath(&ArgV[i + 1], FALSE, &szFile)) {
                        YoriLibInitEmptyString(&szFile);
                    }
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("dump")) == 0) {
                if (i + 1 < ArgC) {
                    op = ScutOperationDump;
                    YoriLibFreeStringContents(&szFile);
                    if (!YoriLibUserStringToSingleFilePath(&ArgV[i + 1], FALSE, &szFile)) {
                        YoriLibInitEmptyString(&szFile);
                    }
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
                    FormatString.StartOfString = ArgV[i + 1].StartOfString;
                    FormatString.LengthInChars = ArgV[i + 1].LengthInChars;
                    FormatString.LengthAllocated = ArgV[i + 1].LengthAllocated;
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("hotkey")) == 0) {
                if (i + 1 < ArgC) {
                    llTemp = 0;
                    if (YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed) &&
                        CharsConsumed > 0) {

                        wHotkey = (WORD)llTemp;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("iconpath")) == 0) {
                if (i + 1 < ArgC) {
                    YoriLibFreeStringContents(&szIcon);
                    if (!YoriLibUserStringToSingleFilePath(&ArgV[i + 1], FALSE, &szIcon)) {
                        YoriLibInitEmptyString(&szIcon);
                    }
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("iconindex")) == 0) {
                if (i + 1 < ArgC) {
                    llTemp = 0;
                    if (YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed) &&
                        CharsConsumed > 0) {

                        wIcon = (WORD)llTemp;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("show")) == 0) {
                if (i + 1 < ArgC) {
                    llTemp = 0;
                    if (YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed) &&
                        CharsConsumed > 0) {

                        wShow = (WORD)llTemp;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
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
                    if (!YoriLibUserStringToSingleFilePath(&ArgV[i + 1], FALSE, &szWorkingDir)) {
                        YoriLibInitEmptyString(&szWorkingDir);
                    }
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

    //
    //  This doesn't exist on original NT4.  Don't explode if it's
    //  missing.
    //

    scut->Vtbl->QueryInterface(scut, &IID_IShellLinkDataList, (void **)&ShortcutDataList);

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

        if (ShortcutDataList != NULL) {
            hRes = ShortcutDataList->Vtbl->CopyDataBlock(ShortcutDataList, ISHELLLINKDATALIST_CONSOLE_PROPS_SIG, &ConsoleProps);
        }

    }

    if (op == ScutOperationDump) {
        YORI_STRING DisplayString;
        SCUT_EXPAND_CONTEXT ExpandContext;

        if (FormatString.StartOfString == NULL) {
            YoriLibConstantString(&FormatString, DefaultFormatString);
        }

        YoriLibInitEmptyString(&DisplayString);
        ExpandContext.ShellLink = scut;
        ExpandContext.ConsoleProperties = ConsoleProps;
        YoriLibExpandCommandVariables(&FormatString, '$', FALSE, ScutExpandVariables, &ExpandContext, &DisplayString);

        if (DisplayString.StartOfString != NULL) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
            YoriLibFreeStringContents(&DisplayString);
            ExitCode = EXIT_SUCCESS;
        }

        if (FormatString.StartOfString == DefaultFormatString &&
            ConsoleProps != NULL) {

            YoriLibConstantString(&FormatString, ConsoleFormatString);
            DisplayString.LengthInChars = 0;
            YoriLibExpandCommandVariables(&FormatString, '$', FALSE, ScutExpandVariables, &ExpandContext, &DisplayString);

            if (DisplayString.StartOfString != NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
                YoriLibFreeStringContents(&DisplayString);
                ExitCode = EXIT_SUCCESS;
            }

            YoriLibConstantString(&FormatString, ConsoleFormatString2);
            DisplayString.LengthInChars = 0;
            YoriLibExpandCommandVariables(&FormatString, '$', FALSE, ScutExpandVariables, &ExpandContext, &DisplayString);

            if (DisplayString.StartOfString != NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
                YoriLibFreeStringContents(&DisplayString);
                ExitCode = EXIT_SUCCESS;
            }
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

    if (op == ScutOperationModify && DeleteConsoleSettings && ShortcutDataList != NULL) {
        if (ShortcutDataList == NULL) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("scut: OS support not present\n"));
            goto Exit;
        } else {
            hRes = ShortcutDataList->Vtbl->RemoveDataBlock(ShortcutDataList, ISHELLLINKDATALIST_CONSOLE_PROPS_SIG);
            if (!SUCCEEDED(hRes)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("RemoveDataBlock failure: %x\n"), hRes);
                goto Exit;
            }
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

    if (ConsoleProps != NULL) {
        LocalFree(ConsoleProps);
    }

    YoriLibFreeStringContents(&szFile);
    YoriLibFreeStringContents(&szIcon);
    YoriLibFreeStringContents(&szWorkingDir);

    return ExitCode;
}

// vim:sw=4:ts=4:et:
