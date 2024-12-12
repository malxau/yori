/**
 * @file scut/scut.c
 *
 * A command line tool to manipulate shortcuts
 *
 * Copyright (c) 2004-2024 Malcolm Smith
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
        "SCUT -create|-modify <filename> [-target target] [-args args] [-autoposition]\n"
        "     [-bold] [-buffersize X*Y] [-desc description] [-deleteconsolesettings]\n"
        "     [-deleteinstallersettings] [-elevate] [-font name] [-fontsize size]\n"
        "     [-hotkey hotkey] [-iconpath filename [-iconindex index]] [-noelevate]\n"
        "     [-nonbold] [-scheme file] [-show showcmd] [-windowposition X*Y]\n"
        "     [-windowsize X*Y] [-workingdir workingdir]\n"
        "SCUT -exec <filename> [-target target] [-args args] [-show showcmd]\n"
        "     [-workingdir workingdir]\n"
        "SCUT [-f fmt] -dump <filename>\n"
        "\n";

/**
 More help text for this application.
 */
const
CHAR strScutHelpText2[] =
        "Format specifiers are:\n"
        "   $ARGS$            Arguments to pass to the target\n"
        "   $AUTOPOSITION$    Whether the system should determine console position\n"
        "   $COLOR_BLACK$     The RGB value to use for console black text\n"
        "   $COLOR_BLUE$      The RGB value to use for console blue text\n"
        "   $COLOR_GREEN$     The RGB value to use for console green text\n"
        "   $COLOR_CYAN$      The RGB value to use for console cyan text\n"
        "   $COLOR_RED$       The RGB value to use for console red text\n"
        "   $COLOR_MAGENTA$   The RGB value to use for console magenta text\n"
        "   $COLOR_BROWN$     The RGB value to use for console brown text\n"
        "   $COLOR_GRAY$      The RGB value to use for console light gray text\n"
        "   $COLOR_DARKGRAY$  The RGB value to use for console dark gray text\n"
        "   $COLOR_LIGHTBLUE$ The RGB value to use for console light blue text\n"
        "   $COLOR_LIGHTGREEN$\n"
        "                     The RGB value to use for console light green text\n"
        "   $COLOR_LIGHTCYAN$ The RGB value to use for console light cyan text\n"
        "   $COLOR_LIGHTRED$  The RGB value to use for console light red text\n"
        "   $COLOR_LIGHTMAGENTA$"
        "                     The RGB value to use for console light magenta text\n"
        "   $COLOR_YELLOW$    The RGB value to use for console yellow text\n"
        "   $COLOR_WHITE$     The RGB value to use for console white text\n";

/**
 More help text for this application.
 */
const
CHAR strScutHelpText3[] =
        "   $CURSORSIZE$      The height of the cursor for console programs\n"
        "   $FONT$            The name of the font to use for console programs\n"
        "   $FONTFAMILY$      The type of font to use for console programs\n"
        "   $FONTNUMBER$      The font index to use for console programs\n"
        "   $FONTWEIGHT$      The boldness of the font to use for console programs\n"
        "   $FULLSCREEN$      Whether a console should be windowed or fullscreen\n"
        "   $HISTORYBUFFERCOUNT$\n"
        "                     The number of console command history buffers\n"
        "   $HISTORYBUFFERSIZE$\n"
        "                     The size of each console command history buffer\n"
        "   $HOTKEY$          Hotkey to open the shortcut\n"
        "   $ICONINDEX$       Zero based index for the icon number within the icon file\n"
        "   $ICONPATH$        Path to the file containing the icon\n"
        "   $INSERT$          Whether insert mode should be initially set on console\n"
        "   $INSTALLERID$     Windows installer identifier for the shortcut\n"
        "   $INSTALLERTARGET$ Program to open from Windows installer ID\n"
        "   $NOHISTORYDUPLICATES$\n"
        "                     Remove identical entries from command history\n"
        "   $POPUPCOLOR$      The color of popup text to use for console programs\n";

/**
 More help text for this application.
 */
const
CHAR strScutHelpText4[] =
        "   $TARGET$          The target of the shortcut\n"
        "   $SCREENBUFFERSIZE_X$\n"
        "                     The width of the console screen buffer\n"
        "   $SCREENBUFFERSIZE_Y$\n"
        "                     The height of the console screen buffer\n"
        "   $SHOW$            The initial state of the window\n"
        "   $WINDOWCOLOR$     The initial color to use for console programs\n"
        "   $WINDOWPOSITION_X$\n"
        "                     The horizontal location of the console window\n"
        "   $WINDOWPOSITION_Y$\n"
        "                     The vertical location of the console window\n"
        "   $SCREENBUFFERSIZE_X$\n"
        "                     The width of the console screen buffer\n"
        "   $SCREENBUFFERSIZE_Y$\n"
        "                     The height of the console screen buffer\n"
        "   $WINDOWSIZE_X$    The width of the console window\n"
        "   $WINDOWSIZE_Y$    The height of the console window\n"
        "   $WORKINGDIR$      The initial working directory\n";

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
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strScutHelpText2);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strScutHelpText3);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strScutHelpText4);
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
     Shell data list flags attached to the shortcut.  May be -1 if flags
     have not been loaded or are not meaningful.
     */
    DWORD ShortcutFlags;

    /**
     Optionally points to extra console properties.  May be NULL if the
     shortcut doesn't specify console properties.
     */
    PISHELLLINKDATALIST_CONSOLE_PROPS ConsoleProperties;

    /**
     Optionally points to extra MSI properties.  May be NULL if the
     shortcut doesn't specify Installer properties.
     */
    PISHELLLINKDATALIST_MSI_PROPS MsiProperties;

    /**
     Points to an MSI target path.  May be empty string.
     */
    PYORI_STRING MsiTarget;

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
YORI_ALLOC_SIZE_T
ScutExpandVariables(
    __inout PYORI_STRING OutputString,
    __in PYORI_STRING VariableName,
    __in PVOID Context
    )
{
    TCHAR szDisplay[MAX_PATH];
    PYORI_STRING yDisplay;
    YORI_ALLOC_SIZE_T CharsNeeded;
    DWORD dwDisplay = 0;
    int iTemp;
    BOOLEAN Numeric = FALSE;
    BOOLEAN UseYoriString = FALSE;
    YORI_ALLOC_SIZE_T HexDigits = 0;
    PSCUT_EXPAND_CONTEXT ExpandContext;
    IShellLinkW * scut;
    PISHELLLINKDATALIST_CONSOLE_PROPS ConsoleProps;
    PISHELLLINKDATALIST_MSI_PROPS MsiProps;
    PYORI_STRING MsiTarget;

    yDisplay = NULL;

    ExpandContext = (PSCUT_EXPAND_CONTEXT)Context;
    scut = ExpandContext->ShellLink;
    ConsoleProps = ExpandContext->ConsoleProperties;
    MsiProps = ExpandContext->MsiProperties;
    MsiTarget = ExpandContext->MsiTarget;

    if (YoriLibCompareStringLit(VariableName, _T("TARGET")) == 0) {
        if (scut->Vtbl->GetPath(scut, szDisplay, MAX_PATH, NULL, 0) != NOERROR) {
            return 0;
        }
    } else if (YoriLibCompareStringLit(VariableName, _T("ARGS")) == 0) {
        if (scut->Vtbl->GetArguments(scut, szDisplay, MAX_PATH) != NOERROR) {
            return 0;
        }
    } else if (YoriLibCompareStringLit(VariableName, _T("WORKINGDIR")) == 0) {
        if (scut->Vtbl->GetWorkingDirectory(scut, szDisplay, MAX_PATH) != NOERROR) {
            return 0;
        }
    } else if (YoriLibCompareStringLit(VariableName, _T("DESCRIPTION")) == 0) {
        if (scut->Vtbl->GetDescription(scut, szDisplay, MAX_PATH) != NOERROR) {
            return 0;
        }
    } else if (YoriLibCompareStringLit(VariableName, _T("ICONPATH")) == 0) {

        if (scut->Vtbl->GetIconLocation(scut, szDisplay, MAX_PATH, &iTemp) != NOERROR) {
            return 0;
        }
    } else if (YoriLibCompareStringLit(VariableName, _T("ICONINDEX")) == 0) {
        if (scut->Vtbl->GetIconLocation(scut, szDisplay, MAX_PATH, &iTemp) != NOERROR) {
            return 0;
        }
        dwDisplay = iTemp;
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("SHOW")) == 0) {
        if (scut->Vtbl->GetShowCmd(scut, &iTemp) != NOERROR) {
            return 0;
        }
        dwDisplay = iTemp;
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("HOTKEY")) == 0) {
        USHORT ShortTemp;
        if (scut->Vtbl->GetHotkey(scut, &ShortTemp) != NOERROR) {
            return 0;
        }
        dwDisplay = ShortTemp;
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("ELEVATE")) == 0) {
        if (ExpandContext->ShortcutFlags != (DWORD)-1 &&
            ExpandContext->ShortcutFlags & SHELLDATALIST_FLAG_RUNASADMIN) {
            dwDisplay = 1;
        } else {
            dwDisplay = 0;
        }
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("INSTALLERID")) == 0) {
        if (MsiProps == NULL) {
            return 0;
        }
        memcpy(szDisplay, MsiProps->szwDarwinID, sizeof(MsiProps->szwDarwinID));
    } else if (YoriLibCompareStringLit(VariableName, _T("INSTALLERTARGET")) == 0) {
        yDisplay = MsiTarget;
        UseYoriString = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("WINDOWCOLOR")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->WindowColor;
        HexDigits = 2;
    } else if (YoriLibCompareStringLit(VariableName, _T("POPUPCOLOR")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->PopupColor;
        HexDigits = 2;
    } else if (YoriLibCompareStringLit(VariableName, _T("SCREENBUFFERSIZE_X")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->ScreenBufferSize.X;
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("SCREENBUFFERSIZE_Y")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->ScreenBufferSize.Y;
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("WINDOWSIZE_X")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->WindowSize.X;
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("WINDOWSIZE_Y")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->WindowSize.Y;
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("WINDOWPOSITION_X")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->WindowPosition.X;
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("WINDOWPOSITION_Y")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->WindowPosition.Y;
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("FONTNUMBER")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->FontNumber;
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("INPUTBUFFERSIZE")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->InputBufferSize;
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("FONTSIZE_X")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->FontSize.X;
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("FONTSIZE_Y")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->FontSize.Y;
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("FONTFAMILY")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->FontFamily;
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("FONTWEIGHT")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->FontWeight;
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("FONT")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        YoriLibSPrintfS(szDisplay, sizeof(szDisplay)/sizeof(szDisplay[0]), _T("%s"), ConsoleProps->FaceName);
    } else if (YoriLibCompareStringLit(VariableName, _T("CURSORSIZE")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->CursorSize;
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("FULLSCREEN")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->FullScreen;
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("QUICKEDIT")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->QuickEdit;
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("INSERT")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->InsertMode;
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("AUTOPOSITION")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->AutoPosition;
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("HISTORYBUFFERSIZE")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->HistoryBufferSize;
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("HISTORYBUFFERCOUNT")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->NumberOfHistoryBuffers;
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("HISTORYBUFFERCOUNT")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->NumberOfHistoryBuffers;
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("NOHISTORYDUPLICATES")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = ConsoleProps->RemoveHistoryDuplicates;
        Numeric = TRUE;
    } else if (YoriLibCompareStringLit(VariableName, _T("COLOR_BLACK")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[0]);
        HexDigits = 6;
    } else if (YoriLibCompareStringLit(VariableName, _T("COLOR_BLUE")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_BLUE]);
        HexDigits = 6;
    } else if (YoriLibCompareStringLit(VariableName, _T("COLOR_GREEN")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_GREEN]);
        HexDigits = 6;
    } else if (YoriLibCompareStringLit(VariableName, _T("COLOR_CYAN")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_BLUE|FOREGROUND_GREEN]);
        HexDigits = 6;
    } else if (YoriLibCompareStringLit(VariableName, _T("COLOR_RED")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_RED]);
        HexDigits = 6;
    } else if (YoriLibCompareStringLit(VariableName, _T("COLOR_MAGENTA")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_RED|FOREGROUND_BLUE]);
        HexDigits = 6;
    } else if (YoriLibCompareStringLit(VariableName, _T("COLOR_BROWN")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_RED|FOREGROUND_GREEN]);
        HexDigits = 6;
    } else if (YoriLibCompareStringLit(VariableName, _T("COLOR_GRAY")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE]);
        HexDigits = 6;
    } else if (YoriLibCompareStringLit(VariableName, _T("COLOR_DARKGRAY")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_INTENSITY]);
        HexDigits = 6;
    } else if (YoriLibCompareStringLit(VariableName, _T("COLOR_LIGHTBLUE")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_INTENSITY|FOREGROUND_BLUE]);
        HexDigits = 6;
    } else if (YoriLibCompareStringLit(VariableName, _T("COLOR_LIGHTGREEN")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_INTENSITY|FOREGROUND_GREEN]);
        HexDigits = 6;
    } else if (YoriLibCompareStringLit(VariableName, _T("COLOR_LIGHTCYAN")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_INTENSITY|FOREGROUND_GREEN|FOREGROUND_BLUE]);
        HexDigits = 6;
    } else if (YoriLibCompareStringLit(VariableName, _T("COLOR_LIGHTRED")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_INTENSITY|FOREGROUND_RED]);
        HexDigits = 6;
    } else if (YoriLibCompareStringLit(VariableName, _T("COLOR_LIGHTMAGENTA")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_INTENSITY|FOREGROUND_RED|FOREGROUND_BLUE]);
        HexDigits = 6;
    } else if (YoriLibCompareStringLit(VariableName, _T("COLOR_YELLOW")) == 0) {
        if (ConsoleProps == NULL) {
            return 0;
        }
        dwDisplay = SCUT_INVERT_COLOR_BYTES(ConsoleProps->ColorTable[FOREGROUND_INTENSITY|FOREGROUND_RED|FOREGROUND_GREEN]);
        HexDigits = 6;
    } else if (YoriLibCompareStringLit(VariableName, _T("COLOR_WHITE")) == 0) {
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
    } else if (UseYoriString) {
        CharsNeeded = yDisplay->LengthInChars;
    } else {
        CharsNeeded = (YORI_ALLOC_SIZE_T)_tcslen(szDisplay);
    }
    if (OutputString->LengthAllocated < CharsNeeded) {
        return CharsNeeded;
    }

    if (UseYoriString) {
        memcpy(OutputString->StartOfString, yDisplay->StartOfString, CharsNeeded * sizeof(TCHAR));
    } else {
        memcpy(OutputString->StartOfString, szDisplay, CharsNeeded * sizeof(TCHAR));
    }
    OutputString->LengthInChars = CharsNeeded;
    return CharsNeeded;
}

/**
 Convert an argument string in the form of (X)x(Y) into a COORD structure.
 For example, the string might be "80x25" or "100*4000".

 @param String Pointer to the input string.

 @param Coord On successful completion, updated to contain the numeric form
        of the user specified coordinate.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
ScutStringToCoord(
    __in PCYORI_STRING String,
    __out PCOORD Coord
    )
{
    YORI_MAX_SIGNED_T llTemp;
    YORI_ALLOC_SIZE_T CharsConsumed;
    YORI_STRING Substring;
    SHORT LocalCoordX;

    if (!YoriLibStringToNumber(String, TRUE, &llTemp, &CharsConsumed) || CharsConsumed == 0) {
        return FALSE;
    }

    if (llTemp > MAXSHORT) {
        return FALSE;
    }

    LocalCoordX = (SHORT)llTemp;

    YoriLibInitEmptyString(&Substring);
    Substring.StartOfString = &String->StartOfString[CharsConsumed];
    Substring.LengthInChars = String->LengthInChars - CharsConsumed;

    if (Substring.LengthInChars == 0) {
        return FALSE;
    }

    if (Substring.StartOfString[0] != 'x' && Substring.StartOfString[0] != '*') {
        return FALSE;
    }

    Substring.StartOfString = Substring.StartOfString + 1;
    Substring.LengthInChars = Substring.LengthInChars - 1;

    if (Substring.LengthInChars == 0) {
        return FALSE;
    }

    if (!YoriLibStringToNumber(&Substring, TRUE, &llTemp, &CharsConsumed) || CharsConsumed == 0) {
        return FALSE;
    }

    if (llTemp > MAXSHORT) {
        return FALSE;
    }

    Coord->X = LocalCoordX;
    Coord->Y = (SHORT)llTemp;
    return TRUE;
}

/**
 The default format string to use when displaying shortcut properties.
 */
LPCTSTR ScutDefaultFormatString = _T("Target:                $TARGET$\n")
                                  _T("Arguments:             $ARGS$\n")
                                  _T("Working dir:           $WORKINGDIR$\n")
                                  _T("Description:           $DESCRIPTION$\n")
                                  _T("Icon Path:             $ICONPATH$\n")
                                  _T("Icon Index:            $ICONINDEX$\n")
                                  _T("Show State:            $SHOW$\n")
                                  _T("Hotkey:                $HOTKEY$\n")
                                  _T("Elevate:               $ELEVATE$\n");

/**
 If the default format string is used, an extra default string for installer
 properties.  This is only displayed if MSI properties are present.
 */
LPCTSTR ScutInstallerFormatString = _T("Installer ID:          $INSTALLERID$\n")
                                    _T("Installer Target:      $INSTALLERTARGET$\n");

/**
 If the default format string is used, an extra default string for console
 properties.  This is only displayed if console properties are present.
 */
LPCTSTR ScutConsoleFormatString = _T("Window Color:          $WINDOWCOLOR$\n")
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

/**
 If the default format string is used, an extra default string for console
 properties.  This is only displayed if console properties are present.
 */
LPCTSTR ScutConsoleFormatString2 = _T("Color Black:           $COLOR_BLACK$\n")
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
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    ScutOperations op       = ScutOperationUnknown;
    YORI_STRING szFile;

    TCHAR * szArgs          = NULL;
    TCHAR * szDesc          = NULL;
    TCHAR * szFont          = NULL;
    WORD    wFontWeight     = 0;
    WORD    wHotkey         = (WORD)-1;
    YORI_STRING szIcon;
    WORD    wIcon           = 0;
    WORD    wShow           = (WORD)-1;
    DWORD   dwShortcutFlags = (DWORD)-1;
    TCHAR * szTarget        = NULL;
    YORI_STRING szWorkingDir;
    YORI_STRING szSchemeFile;
    YORI_STRING szMsiTarget;
    IShellLinkW *scut       = NULL;
    IPersistFile *savedfile = NULL;
    IShellLinkDataList *ShortcutDataList = NULL;
    YORI_STRING Arg;
    YORI_MAX_SIGNED_T llTemp;
    YORI_ALLOC_SIZE_T CharsConsumed;
    DWORD   ExitCode;
    COORD   BufferSize;
    COORD   WindowSize;
    COORD   WindowPosition;
    COORD   FontSize;

    HRESULT hRes;
    BOOLEAN ArgumentUnderstood;
    BOOLEAN DeleteConsoleSettings = FALSE;
    BOOLEAN DeleteInstallerSettings = FALSE;
    YORI_ALLOC_SIZE_T i;
    YORI_STRING FormatString;
    PISHELLLINKDATALIST_CONSOLE_PROPS ConsoleProps = NULL;
    PISHELLLINKDATALIST_MSI_PROPS MsiProps = NULL;
    BOOLEAN FreeConsolePropsWithLocalFree = FALSE;
    BOOLEAN FreeConsolePropsWithDereference = FALSE;
    BOOLEAN AutoPositionSet = FALSE;
    BOOLEAN AutoPosition = FALSE;
    BOOLEAN FullScreen = FALSE;
    BOOLEAN FullScreenSet = FALSE;
    BOOLEAN Elevate = FALSE;
    BOOLEAN NoElevate = FALSE;

    YoriLibInitEmptyString(&szFile);
    YoriLibInitEmptyString(&szIcon);
    YoriLibInitEmptyString(&szWorkingDir);
    YoriLibInitEmptyString(&szSchemeFile);
    YoriLibInitEmptyString(&szMsiTarget);
    YoriLibInitEmptyString(&FormatString);
    BufferSize.X = 0;
    BufferSize.Y = 0;
    FontSize.X = 0;
    FontSize.Y = 0;
    WindowPosition.X = 0;
    WindowPosition.Y = 0;
    WindowSize.X = 0;
    WindowSize.Y = 0;

    ExitCode = EXIT_FAILURE;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                ScutHelp();
                ExitCode = EXIT_SUCCESS;
                goto Exit;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2004-2024"));
                ExitCode = EXIT_SUCCESS;
                goto Exit;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("create")) == 0) {
                if (i + 1 < ArgC) {
                    op = ScutOperationCreate;
                    YoriLibFreeStringContents(&szFile);
                    YoriLibUserStringToSingleFilePath(&ArgV[i + 1], FALSE, &szFile);
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("modify")) == 0) {
                if (i + 1 < ArgC) {
                    op = ScutOperationModify;
                    YoriLibFreeStringContents(&szFile);
                    YoriLibUserStringToSingleFilePath(&ArgV[i + 1], FALSE, &szFile);
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("exec")) == 0) {
                if (i + 1 < ArgC) {
                    op = ScutOperationExec;
                    YoriLibFreeStringContents(&szFile);
                    if (!YoriLibUserStringToSingleFilePath(&ArgV[i + 1], FALSE, &szFile)) {
                        YoriLibInitEmptyString(&szFile);
                    }
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("dump")) == 0) {
                if (i + 1 < ArgC) {
                    op = ScutOperationDump;
                    YoriLibFreeStringContents(&szFile);
                    if (!YoriLibUserStringToSingleFilePath(&ArgV[i + 1], FALSE, &szFile)) {
                        YoriLibInitEmptyString(&szFile);
                    }
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("args")) == 0) {
                if (i + 1 < ArgC) {
                    szArgs = ArgV[i + 1].StartOfString;
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("autoposition")) == 0) {
                AutoPosition = TRUE;
                AutoPositionSet = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("bold")) == 0) {
                wFontWeight = FW_BOLD;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("buffersize")) == 0) {
                if (i + 1 < ArgC) {
                    if (ScutStringToCoord(&ArgV[i + 1], &BufferSize)) {
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("desc")) == 0) {
                if (i + 1 < ArgC) {
                    szDesc = ArgV[i + 1].StartOfString;
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("deleteconsolesettings")) == 0) {
                if (op == ScutOperationModify || op == ScutOperationCreate) {
                    DeleteConsoleSettings = TRUE;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("deleteinstallersettings")) == 0) {
                if (op == ScutOperationModify || op == ScutOperationCreate) {
                    DeleteInstallerSettings = TRUE;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("elevate")) == 0) {
                if (op == ScutOperationModify || op == ScutOperationCreate) {
                    Elevate = TRUE;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("f")) == 0) {
                if (i + 1 < ArgC) {
                    FormatString.StartOfString = ArgV[i + 1].StartOfString;
                    FormatString.LengthInChars = ArgV[i + 1].LengthInChars;
                    FormatString.LengthAllocated = ArgV[i + 1].LengthAllocated;
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("font")) == 0) {
                if (i + 1 < ArgC) {
                    szFont = ArgV[i + 1].StartOfString;
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("fontsize")) == 0) {
                if (i + 1 < ArgC) {
                    llTemp = 0;
                    if (ScutStringToCoord(&ArgV[i + 1], &FontSize)) {
                        ArgumentUnderstood = TRUE;
                        i++;
                    } else if (YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed) &&
                               CharsConsumed > 0) {
                        FontSize.X = 0;
                        FontSize.Y = (WORD)llTemp;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("fullscreen")) == 0) {
                FullScreen = TRUE;
                FullScreenSet = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("hotkey")) == 0) {
                if (i + 1 < ArgC) {
                    llTemp = 0;
                    if (YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed) &&
                        CharsConsumed > 0) {

                        wHotkey = (WORD)llTemp;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("iconpath")) == 0) {
                if (i + 1 < ArgC) {
                    YoriLibFreeStringContents(&szIcon);
                    if (!YoriLibUserStringToSingleFilePath(&ArgV[i + 1], FALSE, &szIcon)) {
                        YoriLibInitEmptyString(&szIcon);
                    }
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("iconindex")) == 0) {
                if (i + 1 < ArgC) {
                    llTemp = 0;
                    if (YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed) &&
                        CharsConsumed > 0) {

                        wIcon = (WORD)llTemp;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("noelevate")) == 0) {
                if (op == ScutOperationModify || op == ScutOperationCreate) {
                    NoElevate = TRUE;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("nonbold")) == 0) {
                wFontWeight = FW_NORMAL;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("scheme")) == 0) {
                if (i + 1 < ArgC) {
                    YoriLibFreeStringContents(&szSchemeFile);
                    if (!YoriLibUserStringToSingleFilePath(&ArgV[i + 1], FALSE, &szSchemeFile)) {
                        YoriLibInitEmptyString(&szSchemeFile);
                    }
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("show")) == 0) {
                if (i + 1 < ArgC) {
                    llTemp = 0;
                    if (YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed) &&
                        CharsConsumed > 0) {

                        wShow = (WORD)llTemp;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("target")) == 0) {
                if (i + 1 < ArgC) {
                    szTarget = ArgV[i + 1].StartOfString;
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("windowed")) == 0) {
                FullScreen = FALSE;
                FullScreenSet = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("windowposition")) == 0) {
                if (i + 1 < ArgC) {
                    if (ScutStringToCoord(&ArgV[i + 1], &WindowPosition)) {
                        AutoPosition = FALSE;
                        AutoPositionSet = TRUE;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("windowsize")) == 0) {
                if (i + 1 < ArgC) {
                    if (ScutStringToCoord(&ArgV[i + 1], &WindowSize)) {
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("workingdir")) == 0) {
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

    YoriLibLoadAdvApi32Functions();
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

        hRes = savedfile->lpVtbl->Load(savedfile, szFile.StartOfString, OpenMode);
        if (!SUCCEEDED(hRes)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Load failure: %x\n"), hRes);
            goto Exit;
        }

        if (ShortcutDataList != NULL) {
            hRes = ShortcutDataList->Vtbl->GetFlags(ShortcutDataList, &dwShortcutFlags);
            if (!SUCCEEDED(hRes)) {
                dwShortcutFlags = (DWORD)-1;
            }
            hRes = ShortcutDataList->Vtbl->CopyDataBlock(ShortcutDataList, ISHELLLINKDATALIST_CONSOLE_PROPS_SIG, &ConsoleProps);
            if (SUCCEEDED(hRes)) {
                ASSERT(ConsoleProps != NULL);
                FreeConsolePropsWithLocalFree = TRUE;
            }

            hRes = ShortcutDataList->Vtbl->CopyDataBlock(ShortcutDataList, ISHELLLINKDATALIST_MSI_PROPS_SIG, &MsiProps);
            if (SUCCEEDED(hRes)) {
                ASSERT(MsiProps != NULL);

                //
                //  NULL terminate this thing since it's just a block of
                //  potentially malicious data
                //

                MsiProps->szwDarwinID[sizeof(MsiProps->szwDarwinID)/sizeof(MsiProps->szwDarwinID[0]) - 1] = '\0';

                //
                //  See if it's possible to convert the block of data into an
                //  actionable target to execute
                //

                if (DllAdvApi32.pCommandLineFromMsiDescriptor) {
                    DWORD dwResult;
                    DWORD Length;
                    Length = 0;
                    dwResult = DllAdvApi32.pCommandLineFromMsiDescriptor(MsiProps->szwDarwinID, NULL, &Length);
                    if (dwResult != ERROR_SUCCESS) {
                        hRes = HRESULT_FROM_WIN32(dwResult);
                        goto Exit;
                    }
                    if (!YoriLibAllocateString(&szMsiTarget, (YORI_ALLOC_SIZE_T)(Length + 1))) {
                        hRes = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
                        goto Exit;
                    }
                    Length = szMsiTarget.LengthAllocated;
                    dwResult = DllAdvApi32.pCommandLineFromMsiDescriptor(MsiProps->szwDarwinID, szMsiTarget.StartOfString, &Length);
                    if (dwResult != ERROR_SUCCESS) {
                        hRes = HRESULT_FROM_WIN32(dwResult);
                        goto Exit;
                    }
                    szMsiTarget.LengthInChars = (YORI_ALLOC_SIZE_T)Length;
                }
            }
        }
    }

    if (op == ScutOperationDump) {
        YORI_STRING TempFormatString;
        YORI_STRING DisplayString;
        SCUT_EXPAND_CONTEXT ExpandContext;

        if (FormatString.StartOfString == NULL) {
            YoriLibConstantString(&FormatString, ScutDefaultFormatString);
        }

        YoriLibInitEmptyString(&DisplayString);
        ExpandContext.ShellLink = scut;
        ExpandContext.ShortcutFlags = dwShortcutFlags;
        ExpandContext.ConsoleProperties = ConsoleProps;
        ExpandContext.MsiProperties = MsiProps;
        ExpandContext.MsiTarget = &szMsiTarget;
        YoriLibExpandCommandVariables(&FormatString, '$', FALSE, ScutExpandVariables, &ExpandContext, &DisplayString);

        if (DisplayString.StartOfString != NULL) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
            YoriLibFreeStringContents(&DisplayString);
            ExitCode = EXIT_SUCCESS;
        }

        //
        //  If the shortcut contains a Windows installer block, display
        //  anything we can get from it.
        //

        if (FormatString.StartOfString == ScutDefaultFormatString &&
            MsiProps != NULL) {

            YoriLibConstantString(&TempFormatString, ScutInstallerFormatString);
            DisplayString.LengthInChars = 0;
            YoriLibExpandCommandVariables(&TempFormatString, '$', FALSE, ScutExpandVariables, &ExpandContext, &DisplayString);

            if (DisplayString.StartOfString != NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
                YoriLibFreeStringContents(&DisplayString);
                ExitCode = EXIT_SUCCESS;
            }
        }

        //
        //  If the shortcut contains a console properties block, display
        //  anything we can get from it.
        //

        if (FormatString.StartOfString == ScutDefaultFormatString &&
            ConsoleProps != NULL) {

            YoriLibConstantString(&TempFormatString, ScutConsoleFormatString);
            DisplayString.LengthInChars = 0;
            YoriLibExpandCommandVariables(&TempFormatString, '$', FALSE, ScutExpandVariables, &ExpandContext, &DisplayString);

            if (DisplayString.StartOfString != NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
                YoriLibFreeStringContents(&DisplayString);
                ExitCode = EXIT_SUCCESS;
            }

            YoriLibConstantString(&TempFormatString, ScutConsoleFormatString2);
            DisplayString.LengthInChars = 0;
            YoriLibExpandCommandVariables(&TempFormatString, '$', FALSE, ScutExpandVariables, &ExpandContext, &DisplayString);

            if (DisplayString.StartOfString != NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
                YoriLibFreeStringContents(&DisplayString);
                ExitCode = EXIT_SUCCESS;
            }
        }
        goto Exit;
    }

    if (Elevate || NoElevate) {
        if (dwShortcutFlags == (DWORD)-1) {
            dwShortcutFlags = 0;
        }

        if (Elevate) {
            dwShortcutFlags = dwShortcutFlags | SHELLDATALIST_FLAG_RUNASADMIN;
        } else {
            dwShortcutFlags = dwShortcutFlags & ~(SHELLDATALIST_FLAG_RUNASADMIN);
        }
    } 

    if (dwShortcutFlags != (DWORD)-1 &&
        ShortcutDataList != NULL &&
        (op == ScutOperationModify || op == ScutOperationCreate)) {

        hRes = ShortcutDataList->Vtbl->SetFlags(ShortcutDataList, dwShortcutFlags);
        if (hRes != NOERROR && hRes != S_FALSE) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("SetFlags failure\n"));
            goto Exit;
        }
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


    if ((op == ScutOperationModify ||
         op == ScutOperationCreate) &&
        !DeleteConsoleSettings &&
        ShortcutDataList != NULL &&
        (szSchemeFile.StartOfString != NULL ||
         szFont != NULL ||
         wFontWeight != 0 ||
         BufferSize.X != 0 ||
         BufferSize.Y != 0 ||
         FontSize.X != 0 ||
         FontSize.Y != 0 ||
         AutoPositionSet ||
         FullScreenSet ||
         WindowPosition.X != 0 ||
         WindowPosition.Y != 0 ||
         WindowSize.X != 0 ||
         WindowSize.Y != 0)) {

        UCHAR Color;

        if (ConsoleProps == NULL) {
            ConsoleProps = YoriLibAllocateDefaultConsoleProperties();
            if (ConsoleProps == NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("YoriLibAllocateDefaultConsoleProperties failure\n"));
                goto Exit;
            }
            FreeConsolePropsWithDereference = TRUE;
        }

        if (szFont != NULL) {
            YORI_SIGNED_ALLOC_SIZE_T CharsCopied;
            YORI_ALLOC_SIZE_T FaceNameSizeInChars;

            FaceNameSizeInChars = sizeof(ConsoleProps->FaceName)/sizeof(ConsoleProps->FaceName[0]);

            CharsCopied = YoriLibSPrintfS(ConsoleProps->FaceName, FaceNameSizeInChars, _T("%s"), szFont);
            ConsoleProps->FaceName[FaceNameSizeInChars - 1] = '\0';

            //
            //  I'm not really sure why font family is even recorded.
            //  Console fonts have to be monospaced.
            //

            ConsoleProps->FontFamily = FF_MODERN | MONO_FONT;
        }

        if (wFontWeight != 0) {
            ConsoleProps->FontWeight = wFontWeight;
        }

        if (szSchemeFile.StartOfString != NULL) {
            YoriLibLoadColorTableFromScheme(&szSchemeFile, ConsoleProps->ColorTable);
            YoriLibLoadWindowColorFromScheme(&szSchemeFile, &Color);
            ConsoleProps->WindowColor = Color;
            YoriLibLoadPopupColorFromScheme(&szSchemeFile, &Color);
            ConsoleProps->PopupColor = Color;
        }

        if (BufferSize.X != 0 || BufferSize.Y != 0) {
            ConsoleProps->ScreenBufferSize.X = BufferSize.X;
            ConsoleProps->ScreenBufferSize.Y = BufferSize.Y;
        }

        if (FontSize.X != 0 || FontSize.Y != 0) {
            ConsoleProps->FontSize.X = FontSize.X;
            ConsoleProps->FontSize.Y = FontSize.Y;
        }

        if (WindowSize.X != 0 || WindowSize.Y != 0) {
            ConsoleProps->WindowSize.X = WindowSize.X;
            ConsoleProps->WindowSize.Y = WindowSize.Y;
        }

        if (AutoPositionSet) {
            ConsoleProps->AutoPosition = AutoPosition;
            if (AutoPosition == FALSE) {
                ConsoleProps->WindowPosition.X = WindowPosition.X;
                ConsoleProps->WindowPosition.Y = WindowPosition.Y;
            }
        }

        if (FullScreenSet) {
            ConsoleProps->FullScreen = FullScreen;
        }

        ShortcutDataList->Vtbl->RemoveDataBlock(ShortcutDataList, ISHELLLINKDATALIST_CONSOLE_PROPS_SIG);
        hRes = ShortcutDataList->Vtbl->AddDataBlock(ShortcutDataList, ConsoleProps);
        if (!SUCCEEDED(hRes)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("AddDataBlock failure: %x\n"), hRes);
            goto Exit;
        }
    }

    if (op == ScutOperationModify &&
        (DeleteConsoleSettings || DeleteInstallerSettings)) {

        if (ShortcutDataList == NULL) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("scut: OS support not present\n"));
            goto Exit;
        }

        if (DeleteConsoleSettings) {
            hRes = ShortcutDataList->Vtbl->RemoveDataBlock(ShortcutDataList, ISHELLLINKDATALIST_CONSOLE_PROPS_SIG);
            if (!SUCCEEDED(hRes)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("RemoveDataBlock failure: %x\n"), hRes);
                goto Exit;
            }
        }

        if (DeleteInstallerSettings) {
            hRes = ShortcutDataList->Vtbl->RemoveDataBlock(ShortcutDataList, ISHELLLINKDATALIST_MSI_PROPS_SIG);
            if (!SUCCEEDED(hRes)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("RemoveDataBlock failure: %x\n"), hRes);
                goto Exit;
            }
        }
    }

    if (op == ScutOperationModify ||
        op == ScutOperationCreate) {
        hRes = savedfile->lpVtbl->Save(savedfile, szFile.StartOfString, TRUE);
        if (!SUCCEEDED(hRes)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Save failure: %x\n"), hRes);
            goto Exit;
        }
    } else if (op == ScutOperationExec) {
        YORI_SHELLEXECUTEINFO sei;
        TCHAR szFileBuf[MAX_PATH];
        TCHAR szArgsBuf[MAX_PATH];
        TCHAR szDir[MAX_PATH];
        INT nShow;
        HINSTANCE hApp;
        LPTSTR ErrText;

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

        ZeroMemory(&sei, sizeof(sei));
        sei.cbSize = sizeof(sei);
        sei.fMask = SEE_MASK_FLAG_NO_UI |
                    SEE_MASK_NOZONECHECKS |
                    SEE_MASK_UNICODE;

        sei.lpFile = szFileBuf;
        sei.lpParameters = szArgsBuf;
        sei.lpDirectory = szDir;
        sei.nShow = nShow;

        if (DllShell32.pShellExecuteExW != NULL) {

            if (dwShortcutFlags != (DWORD)-1 &&
                (dwShortcutFlags & SHELLDATALIST_FLAG_RUNASADMIN) != 0) {
                sei.lpVerb = _T("runas");
            }

            if (!DllShell32.pShellExecuteExW(&sei)) {
                ErrText = YoriLibGetWinErrorText(GetLastError());
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ShellExecuteEx failed: %s: %s"), szFileBuf, ErrText);
                YoriLibFreeWinErrorText(ErrText);
                goto Exit;
            }

            hRes = S_OK;
        } else if (DllShell32.pShellExecuteW != NULL) {
            hApp = DllShell32.pShellExecuteW(NULL,
                                             NULL,
                                             sei.lpFile,
                                             sei.lpParameters,
                                             sei.lpDirectory,
                                             sei.nShow);
            if ((ULONG_PTR)hApp <= 32) {
                ErrText = YoriLibGetWinErrorText(YoriLibShellExecuteInstanceToError(hApp));
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ShellExecute failed: %s: %s"), szFileBuf, ErrText);
                YoriLibFreeWinErrorText(ErrText);
                goto Exit;
            }

            hRes = S_OK;
        }
    }

    ExitCode = EXIT_SUCCESS;

Exit:

    if (MsiProps != NULL) {
        LocalFree(MsiProps);
    }

    if (FreeConsolePropsWithLocalFree) {
        ASSERT(ConsoleProps != NULL);
        LocalFree(ConsoleProps);
    } else if (FreeConsolePropsWithDereference) {
        ASSERT(ConsoleProps != NULL);
        YoriLibDereference(ConsoleProps);
    }

    if (ShortcutDataList != NULL) {
        ShortcutDataList->Vtbl->Release(ShortcutDataList);
    }

    if (scut != NULL) {
        scut->Vtbl->Release(scut);
    }

    YoriLibFreeStringContents(&szFile);
    YoriLibFreeStringContents(&szIcon);
    YoriLibFreeStringContents(&szWorkingDir);
    YoriLibFreeStringContents(&szSchemeFile);
    YoriLibFreeStringContents(&szMsiTarget);

    return ExitCode;
}

// vim:sw=4:ts=4:et:
