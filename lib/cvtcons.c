/**
 * @file lib/cvtcons.c
 *
 * Convert VT100/ANSI escape sequences into a console CHAR_INFO array.
 *
 * Copyright (c) 2015-2021 Malcolm J. Smith
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

#include "yoripch.h"
#include "yorilib.h"

/**
 The default color to use when all else fails.
 */
#define CVTVT_DEFAULT_COLOR (7)

/**
 A context which can be passed around as a "handle" when generating an RTF
 output string from VT100 text.
 */
typedef struct _YORI_LIB_CHARINFO_CONVERT_CONTEXT {

    /**
     A buffer containing the rendered characters and attributes.
     */
    PCHAR_INFO Output;

    /**
     The number of columns in the Output buffer.
     */
    WORD Columns;

    /**
     The current location of the cursor within the Output buffer.
     */
    WORD CursorX;

    /**
     The current location of the cursor within the Output buffer.
     */
    WORD CursorY;

    /**
     The default attributes to use in response to a reset.
     */
    WORD DefaultAttributes;

    /**
     The attributes that are currently applied.
     */
    WORD CurrentAttributes;

    /**
     If TRUE, characters at the end of the line start at the next line.  This
     would be normal for named pipe input.  If FALSE, characters do not move
     to the next line without a new line, which is standard on VT terminals.
     */
    BOOLEAN LineWrap;

    /**
     The number of lines allocated in the Output buffer.
     */
    DWORD LinesAllocated;

    /**
     The maximum number of lines that can be allocated into the Output buffer.
     */
    DWORD MaximumLines;

    /**
     Specifies the callback functions to operate with the buffer.
     */
    YORI_LIB_VT_CALLBACK_FUNCTIONS Callbacks;

} YORI_LIB_CHARINFO_CONVERT_CONTEXT, *PYORI_LIB_CHARINFO_CONVERT_CONTEXT;


/**
 Indicate the beginning of a stream and perform any initial output.

 @param hOutput The context to output any footer information to.

 @param Context Pointer to context.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibConsCnvInitializeStream(
    __in HANDLE hOutput,
    __inout DWORDLONG *Context
    )
{
    // PYORI_LIB_CHARINFO_CONVERT_CONTEXT Context = (PYORI_LIB_CHARINFO_CONVERT_CONTEXT)hOutput;
    UNREFERENCED_PARAMETER(hOutput);
    UNREFERENCED_PARAMETER(Context);

    return TRUE;
}

/**
 Indicate the end of the stream has been reached and perform any final
 output.

 @param hOutput The context to output any footer information to.

 @param Context Pointer to context.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibConsCnvEndStream(
    __in HANDLE hOutput,
    __inout DWORDLONG *Context
    )
{
    // PYORI_LIB_CHARINFO_CONVERT_CONTEXT Context = (PYORI_LIB_CHARINFO_CONVERT_CONTEXT)hOutput;
    UNREFERENCED_PARAMETER(hOutput);
    UNREFERENCED_PARAMETER(Context);
    return TRUE;
}

/**
 Set a value in the output buffer.

 @param CvtContext Pointer to the context containing the buffer and width.

 @param X The horizontal coordinate.

 @param Y The vertical coordinate.

 @param Char The character to add.

 @param Attr The attribute to add.
 */
VOID
YoriLibConsCnvSetCell(
    __inout PYORI_LIB_CHARINFO_CONVERT_CONTEXT CvtContext,
    __in DWORD X,
    __in DWORD Y,
    __in TCHAR Char,
    __in WORD Attr
    )
{
    ASSERT(X < CvtContext->Columns);
    ASSERT(Y < CvtContext->LinesAllocated);

    CvtContext->Output[CvtContext->Columns * Y + X].Char.UnicodeChar = Char;
    CvtContext->Output[CvtContext->Columns * Y + X].Attributes = Attr;
}

/**
 Move the cursor to the next line.  This will fill the end of the line with
 empty characters.

 @param CvtContext Pointer to the context.

 @return TRUE to indicate that the cursor has been moved successfully. FALSE
         to indicate that the end of the buffer has been reached.
 */
BOOLEAN
YoriLibConsCnvMoveToNextLine(
    __inout PYORI_LIB_CHARINFO_CONVERT_CONTEXT CvtContext
    )
{
    //
    //  MSFIX
    //   - Possibly reallocate
    //

    for (; CvtContext->CursorX < CvtContext->Columns; CvtContext->CursorX++) {
        YoriLibConsCnvSetCell(CvtContext, CvtContext->CursorX, CvtContext->CursorY, ' ', CvtContext->CurrentAttributes);
    }

    if ((DWORD)(CvtContext->CursorY + 1) < CvtContext->LinesAllocated) {
        CvtContext->CursorX = 0;
        CvtContext->CursorY = (WORD)(CvtContext->CursorY + 1);
        return TRUE;
    }
    return FALSE;
}

/**
 Parse text between VT100 escape sequences and generate correct output for
 either RTF.

 @param hOutput The output context to populate with the translated text
        information.

 @param String Pointer to a string buffer containing the text to process.

 @param Context Pointer to context.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibConsCnvProcessAndOutputText(
    __in HANDLE hOutput,
    __in PCYORI_STRING String,
    __inout DWORDLONG *Context
    )
{
    PYORI_LIB_CHARINFO_CONVERT_CONTEXT CvtContext;
    DWORD Index;
    TCHAR Char;

    UNREFERENCED_PARAMETER(hOutput);

    CvtContext = (PYORI_LIB_CHARINFO_CONVERT_CONTEXT)(DWORD_PTR)(*Context);
    for (Index = 0; Index < String->LengthInChars; Index++) {
        Char = String->StartOfString[Index];
        switch(Char) {
            case '\r':
                continue;
            case '\n':
                if (!YoriLibConsCnvMoveToNextLine(CvtContext)) {
                    return FALSE;
                }
                break;
            default:
                if (CvtContext->CursorX == CvtContext->Columns) {
                    if (!CvtContext->LineWrap) {
                        continue;
                    }
                    if (!YoriLibConsCnvMoveToNextLine(CvtContext)) {
                        return FALSE;
                    }
                }

                YoriLibConsCnvSetCell(CvtContext, CvtContext->CursorX, CvtContext->CursorY, Char, CvtContext->CurrentAttributes);
                CvtContext->CursorX++;
                break;
        }
    }


    return TRUE;
}


/**
 Parse a VT100 escape sequence and generate the correct output for RTF.

 @param hOutput The output context to populate with the translated escape
        information.

 @param String Pointer to a string buffer containing the escape to process.

 @param Context Pointer to context.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibConsCnvProcessAndOutputEscape(
    __in HANDLE hOutput,
    __in PCYORI_STRING String,
    __inout DWORDLONG *Context
    )
{
    PYORI_LIB_CHARINFO_CONVERT_CONTEXT CvtContext;
    CvtContext = (PYORI_LIB_CHARINFO_CONVERT_CONTEXT)(DWORD_PTR)(*Context);

    UNREFERENCED_PARAMETER(hOutput);

    YoriLibVtFinalColorFromSequence(CvtContext->CurrentAttributes, String, &CvtContext->CurrentAttributes);
    return TRUE;
}

/**
 Display the contents of the buffer a a specified location on the display.

 @param Context Pointer to the context specifying the text and attributes.

 @param hConsole Handle to the console.

 @param X Specifies the horizontal coordinate to display text.

 @param Y Specifies the vertical coordinate to display text.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriLibConsoleVtConvertDisplayToConsole(
    __in PVOID Context,
    __in HANDLE hConsole,
    __in DWORD X,
    __in DWORD Y
    )
{
    PYORI_LIB_CHARINFO_CONVERT_CONTEXT CvtContext;
    COORD BufferSize;
    COORD BufferCoord;
    SMALL_RECT OutputRect;

    CvtContext = (PYORI_LIB_CHARINFO_CONVERT_CONTEXT)CONTAINING_RECORD(Context, YORI_LIB_CHARINFO_CONVERT_CONTEXT, Callbacks);
    BufferSize.X = CvtContext->Columns;
    BufferSize.Y = CvtContext->CursorY;
    BufferCoord.X = 0;
    BufferCoord.Y = 0;
    OutputRect.Left = (SHORT)X;
    OutputRect.Top = (SHORT)Y;
    OutputRect.Right = (SHORT)(X + CvtContext->Columns);
    OutputRect.Bottom = (SHORT)(Y + CvtContext->CursorY);

    WriteConsoleOutput(hConsole, CvtContext->Output, BufferSize, BufferCoord, &OutputRect);
    return TRUE;
}


/**
 Return the cursor location within the buffer.

 @param Context Pointer to the context containing the cursor location.

 @param X On successful completion, updated to contain the horizontal cursor
        location.

 @param Y On successful completion, updated to contain the vertical cursor
        location.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriLibConsoleVtConvertGetCursorLocation(
    __in PVOID Context,
    __out PDWORD X,
    __out PDWORD Y
    )
{
    PYORI_LIB_CHARINFO_CONVERT_CONTEXT CvtContext;
    CvtContext = (PYORI_LIB_CHARINFO_CONVERT_CONTEXT)CONTAINING_RECORD(Context, YORI_LIB_CHARINFO_CONVERT_CONTEXT, Callbacks);

    *X = CvtContext->CursorX;
    *Y = CvtContext->CursorY;

    return TRUE;
}


/**
 Allocate a new context structure.

 @param Columns The number of columns to render into.

 @param AllocateLines The number of lines to allocate initially.  If these
        cannot be allocated, the call fails.

 @param MaximumLines The number of lines to allocate on demand.

 @param DefaultAttributes The attributes to apply on reset.

 @param CurrentAttributes The attributes to apply from the beginning of the
        output.

 @param LineWrap If TRUE, output from the end of one line starts on the next
        line.  If FALSE, output at the end of a line is ignored until a new
        line character is encountered.

 @return Pointer to the context, or NULL on allocation failure.
 */
PVOID
YoriLibAllocateConsoleVtConvertContext(
    __in WORD Columns,
    __in DWORD AllocateLines,
    __in DWORD MaximumLines,
    __in WORD DefaultAttributes,
    __in WORD CurrentAttributes,
    __in BOOLEAN LineWrap
    )
{
    PYORI_LIB_CHARINFO_CONVERT_CONTEXT Context;

    Context = YoriLibReferencedMalloc(sizeof(YORI_LIB_CHARINFO_CONVERT_CONTEXT));
    if (Context == NULL) {
        return NULL;
    }

    Context->Columns = Columns;
    Context->CursorX = 0;
    Context->CursorY = 0;
    Context->DefaultAttributes = DefaultAttributes;
    Context->CurrentAttributes = CurrentAttributes;

    Context->MaximumLines = MaximumLines;
    Context->LinesAllocated = AllocateLines;
    Context->Output = NULL;
    Context->LineWrap = LineWrap;

    if (AllocateLines > 0) {
        DWORD BytesNeeded;
        BytesNeeded = Columns * AllocateLines * sizeof(CHAR_INFO);
        if (!YoriLibIsSizeAllocatable(BytesNeeded)) {
            YoriLibDereference(Context);
            return NULL;
        }

        Context->Output = YoriLibReferencedMalloc((YORI_ALLOC_SIZE_T)BytesNeeded);
        if (Context->Output == NULL) {
            YoriLibDereference(Context);
            return NULL;
        }
    }

    Context->Callbacks.InitializeStream = YoriLibConsCnvInitializeStream;
    Context->Callbacks.EndStream = YoriLibConsCnvEndStream;
    Context->Callbacks.ProcessAndOutputText = YoriLibConsCnvProcessAndOutputText;
    Context->Callbacks.ProcessAndOutputEscape = YoriLibConsCnvProcessAndOutputEscape;
    Context->Callbacks.Context = (DWORDLONG)(DWORD_PTR)Context;

    return &Context->Callbacks;
}

// vim:sw=4:ts=4:et:
