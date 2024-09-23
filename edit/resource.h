/**
 * @file edit/resource.h
 *
 * Yori editor resource definitions.  This file must be usablefrom RC as well
 * as C.
 *
 * Copyright (c) 2024 Malcolm J. Smith
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



//
// ==================================================================
//
//                        !!! IMPORTANT !!!
//
// The following list must be fully contiguous with no gaps.  These
// are loaded as an array, where the _MAX constant at the end
// specifies the array length.
// ==================================================================
//
 
 
/**
 The string ID for the File menu item.
 */
#define IDS_YEDIT_MNU_FILE                  0

/**
 The string ID for the File, New menu item.
 */
#define IDS_YEDIT_MNU_FILE_NEW              1

/**
 The string ID for the File, Open menu item.
 */
#define IDS_YEDIT_MNU_FILE_OPEN             2

/**
 The string ID for the File, Save menu item.
 */
#define IDS_YEDIT_MNU_FILE_SAVE             3

/**
 The string ID for the File, Save As menu item.
 */
#define IDS_YEDIT_MNU_FILE_SAVEAS           4

/**
 The string ID for the File, Exit menu item.
 */
#define IDS_YEDIT_MNU_FILE_EXIT             5

/**
 The string ID for the Edit menu item.
 */
#define IDS_YEDIT_MNU_EDIT                  6

/**
 The string ID for the Edit, Undo menu item.
 */
#define IDS_YEDIT_MNU_EDIT_UNDO             7

/**
 The string ID for the Edit, Redo menu item.
 */
#define IDS_YEDIT_MNU_EDIT_REDO             8

/**
 The string ID for the Edit, Cut menu item.
 */
#define IDS_YEDIT_MNU_EDIT_CUT              9

/**
 The string ID for the Edit, Copy menu item.
 */
#define IDS_YEDIT_MNU_EDIT_COPY             10

/**
 The string ID for the Edit, Paste menu item.
 */
#define IDS_YEDIT_MNU_EDIT_PASTE            11

/**
 The string ID for the Edit, Clear menu item.
 */
#define IDS_YEDIT_MNU_EDIT_CLEAR            12

/**
 The string ID for the Search menu item.
 */
#define IDS_YEDIT_MNU_SEARCH                13

/**
 The string ID for the Search, Find menu item.
 */
#define IDS_YEDIT_MNU_SEARCH_FIND           14

/**
 The string ID for the Search, Repeat menu item.
 */
#define IDS_YEDIT_MNU_SEARCH_REPEAT         15

/**
 The string ID for the Search, Previous menu item.
 */
#define IDS_YEDIT_MNU_SEARCH_PREVIOUS       16

/**
 The string ID for the Search, Change menu item.
 */
#define IDS_YEDIT_MNU_SEARCH_CHANGE         17

/**
 The string ID for the Search, Goto menu item.
 */
#define IDS_YEDIT_MNU_SEARCH_GOTO           18

/**
 The string ID for the Options menu item.
 */
#define IDS_YEDIT_MNU_OPTIONS               19

/**
 The string ID for the Options, Display menu item.
 */
#define IDS_YEDIT_MNU_OPTIONS_DISPLAY       20

/**
 The string ID for the Options, Traditional menu item.
 */
#define IDS_YEDIT_MNU_OPTIONS_TRADITIONAL   21

/**
 The string ID for the Options, Auto Indent menu item.
 */
#define IDS_YEDIT_MNU_OPTIONS_AUTOINDENT    22

/**
 The string ID for the Options, Expand Tab menu item.
 */
#define IDS_YEDIT_MNU_OPTIONS_EXPANDTAB     23

/**
 The string ID for the Options, Save Defaults menu item.
 */
#define IDS_YEDIT_MNU_OPTIONS_SAVEDEFAULTS  24

/**
 The string ID for the Help menu item.
 */
#define IDS_YEDIT_MNU_HELP                  25

/**
 The string ID for the Help, About menu item.
 */
#define IDS_YEDIT_MNU_HELP_ABOUT            26

/**
 The string ID for the UTF detect based on BOM encoding.
 */
#define IDS_YEDIT_ENC_UTF_DETECT            27

/**
 The string ID for the UTF8 encoding.
 */
#define IDS_YEDIT_ENC_UTF8                  28

/**
 The string ID for the ANSI encoding.
 */
#define IDS_YEDIT_ENC_ANSI                  29

/**
 The string ID for the ASCII encoding.
 */
#define IDS_YEDIT_ENC_ASCII                 30

/**
 The string ID for the UTF16 encoding.
 */
#define IDS_YEDIT_ENC_UTF16                 31

/**
 The string ID for the UTF8 with BOM encoding.
 */
#define IDS_YEDIT_ENC_UTF8_BOM              32

/**
 The string ID for the UTF16 with BOM encoding.
 */
#define IDS_YEDIT_ENC_UTF16_BOM             33

/**
 The string ID for the CRLF line ending.
 */
#define IDS_YEDIT_LINE_CRLF                 34

/**
 The string ID for the LF line ending.
 */
#define IDS_YEDIT_LINE_LF                   35

/**
 The string ID for the CR line ending.
 */
#define IDS_YEDIT_LINE_CR                   36

/**
 The string ID for open for editing.
 */
#define IDS_YEDIT_OPEN_EDITING              37

/**
 The string ID for open readonly.
 */
#define IDS_YEDIT_OPEN_READONLY             38

/**
 The string ID for the dialog caption for open readonly.
 */
#define IDS_YEDIT_DLG_READONLY              39

/**
 The string ID for the dialog caption for encoding.
 */
#define IDS_YEDIT_DLG_ENCODING              40

/**
 The string ID for the dialog caption for line ending.
 */
#define IDS_YEDIT_DLG_LINEENDING            41

/**
 The string ID for the button text for Ok.
 */
#define IDS_YEDIT_BUTTON_OK                 42

/**
 The string ID for the button text for Yes.
 */
#define IDS_YEDIT_BUTTON_YES                43

/**
 The string ID for the button text for No.
 */
#define IDS_YEDIT_BUTTON_NO                 44

/**
 The string ID for the button text for Cancel.
 */
#define IDS_YEDIT_BUTTON_CANCEL             45

/**
 The string ID for the button text for View license.
 */
#define IDS_YEDIT_BUTTON_LICENSE            46

/**
 The string ID for the button text for Overwrite file.
 */
#define IDS_YEDIT_BUTTON_OVERWRITE          47

/**
 The string ID for the button text for Don't Save.
 */
#define IDS_YEDIT_BUTTON_DONTSAVE           48

/**
 The string ID for the dialog title for Open.
 */
#define IDS_YEDIT_TITLE_OPEN                49

/**
 The string ID for the dialog title for Save.
 */
#define IDS_YEDIT_TITLE_SAVE                50

/**
 The string ID for the dialog title for Save As.
 */
#define IDS_YEDIT_TITLE_SAVEAS              51

/**
 The string ID for the dialog title for Find.
 */
#define IDS_YEDIT_TITLE_FIND                52

/**
 The string ID for the dialog title for Go to line.
 */
#define IDS_YEDIT_TITLE_GOTO                53

/**
 The string ID for the dialog title for About.
 */
#define IDS_YEDIT_TITLE_ABOUT               54

/**
 The string ID for the dialog title for View License.
 */
#define IDS_YEDIT_TITLE_LICENSE             55

/**
 The string ID for the dialog title for Error.
 */
#define IDS_YEDIT_TITLE_ERROR               56

/**
 The string ID for the dialog title for Save Changes.
 */
#define IDS_YEDIT_TITLE_SAVECHANGES         57

/**
 The string ID for the dialog title for Overwrite read only.
 */
#define IDS_YEDIT_TITLE_OVERWRITEREADONLY   58

/**
 The string ID for text not found.
 */
#define IDS_YEDIT_FIND_NOTFOUND             59

/**
 The string ID for no more matches found.
 */
#define IDS_YEDIT_FIND_NOMOREFOUND          60

/**
 The string ID for failure to open file.
 */
#define IDS_YEDIT_FAIL_OPENFILE             61

/**
 The string ID for failure to open file for write.
 */
#define IDS_YEDIT_FAIL_OPENFILEWRITE        62

/**
 The string ID for failure to clear read only.
 */
#define IDS_YEDIT_FAIL_CLEARREADONLY        63

/**
 The string ID for failure to open registry key.
 */
#define IDS_YEDIT_FAIL_OPENREGISTRY         64

/**
 The string ID for registry not supported.
 */
#define IDS_YEDIT_FAIL_REGISTRYSUPPORT      65

/**
 The string ID for window too small.
 */
#define IDS_YEDIT_MSG_WINDOWTOOSMALL        66

/**
 The string ID for file modified (prompt to save.)
 */
#define IDS_YEDIT_MSG_FILEMODIFIED          67

/**
 The string ID for file read only (prompt to clear.)
 */
#define IDS_YEDIT_MSG_FILEREADONLY          68

/**
 The number of consecutive string IDs above.
 */
#define IDS_YEDIT_MAX                       69
