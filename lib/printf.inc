/**
 * @file printf.inc
 *
 * Implementation for the core printf engine.
 *
 * Copyright (c) 2014-2017 Malcolm J. Smith
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


#ifdef PRINTF_FN
#undef PRINTF_FN
#undef PRINTF_DESTLENGTH
#undef PRINTF_PUSHCHAR
#endif

#define PRINTF_ANSI_TO_UNICODE(x)     (TCHAR)((UCHAR)(x))
#define PRINTF_UNICODE_TO_ANSI(x)     (TCHAR)(((x>=0x20&&x<0x80)||(x=='\r')||(x=='\n'))?x:'?')

#ifdef PRINTF_SIZEONLY

//
//  The sizeonly variants are intentionally not manipulating the buffer, which
//  makes the compiler wonder if we intentionally wanted to ignore the
//  buffer operation.
//

#if defined(_MSC_VER) && (_MSC_VER >= 1500)
#pragma warning(disable: 6269) // Possibly incorrect order of operations, dereference ignored
#endif

#define PRINTF_FN YoriLibVSPrintfSize

#define PRINTF_DESTLENGTH() (1)
#define PRINTF_PUSHCHAR(x)  dest_offset++,x;

#else // PRINTF_SIZEONLY

#ifdef UNICODE
#define PRINTF_FN YoriLibVSPrintf
#else
#define PRINTF_FN YoriLibVSPrintfA
#endif

#define PRINTF_DESTLENGTH()  (dest_offset < len - 1)
#define PRINTF_PUSHCHAR(x)   szDest[dest_offset++] = x;

#endif // PRINTF_SIZEONLY

int
PRINTF_FN(
#ifndef PRINTF_SIZEONLY
        __out_ecount(len) LPTSTR szDest,
        __in DWORD len,
#endif
        __in LPCTSTR szFmt,
        __in va_list marker)
{
    DWORD dest_offset = 0;
    DWORD src_offset = 0;
    DWORD i;

    BOOL leadingzero;
    BOOL leftalign;
    BOOL short_prefix;
    BOOL long_prefix;
    BOOL longlong_prefix;
    BOOL truncated_due_to_space;
    DWORD element_len;

    truncated_due_to_space = FALSE;

    while (szFmt[src_offset] != '\0') {
        if (szFmt[src_offset] == '%') {
            src_offset++;
            leadingzero = FALSE;
            leftalign = FALSE;
            long_prefix = FALSE;
            longlong_prefix = FALSE;
            short_prefix = FALSE;
            element_len = 0;

            if (szFmt[src_offset] == '-') {
                leftalign = TRUE;
                src_offset++;
            }
            if (szFmt[src_offset] == '0') {
                leadingzero = TRUE;
                src_offset++;
            }
            while (szFmt[src_offset] >= '0' && szFmt[src_offset] <= '9') {
                element_len = element_len * 10 + szFmt[src_offset] - '0';
                src_offset++;
            }
            if (szFmt[src_offset] == 'h') {
                short_prefix = TRUE;
                src_offset++;
            } else if (szFmt[src_offset] == 'l' && szFmt[src_offset + 1] == 'l') {
                longlong_prefix = TRUE;
                src_offset += 2;
            } else if (szFmt[src_offset] == 'l') {
                long_prefix = TRUE;
                src_offset++;
            }

            if (element_len == 0) {
                element_len = (DWORD) -1;
            }

#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(suppress: 6240) // Conditional is constant
#endif
            if (szFmt[src_offset] == 'p' && sizeof(PVOID) == sizeof(DWORDLONG)) {
                longlong_prefix = TRUE;
            }

            switch(szFmt[src_offset]) {
                case '%':
                    if (PRINTF_DESTLENGTH()) {
                        PRINTF_PUSHCHAR('%');
                    } else {
                        truncated_due_to_space = TRUE;
                    }
                    break;

                case 'c':
                    {
                        //
                        //  The compiler always upconverts chars to ints when
                        //  creating variable arguments.  We have to mirror
                        //  that semantic here.  MSVC gets this "right" by
                        //  allowing these to be symmetrical and upconverting
                        //  both, but gcc gets it "wrong" and explodes by
                        //  upconverting one and not the other (then printing
                        //  a warning blaming this code.)
                        //

                        i = (TCHAR)va_arg(marker, int);
                        if (PRINTF_DESTLENGTH()) {
                            PRINTF_PUSHCHAR((TCHAR)i);
                        } else {
                            truncated_due_to_space = TRUE;
                        }
                    }
                    break;
                case 's':
                    {
                        LPTSTR str = va_arg(marker, LPTSTR);
                        LPSTR  short_str = (LPSTR)str;
                        LPWSTR long_str = (LPWSTR)str;

                        if (str == NULL) {
                            short_str = "(null)";
                            short_prefix = TRUE;
                            long_prefix = FALSE;
                        }

                        if (!short_prefix && !long_prefix) {
#ifdef UNICODE
                            long_prefix = TRUE;
#else
                            short_prefix = TRUE;
#endif
                        }

                        if (element_len != (DWORD)-1 && !leftalign) {
                            DWORD str_len = 0;
                            if (short_prefix) {
                                while (short_str[str_len] != '\0') str_len++;
                            } else if (long_prefix) {
                                while (long_str[str_len] != '\0') str_len++;
                            }

                            while (element_len > str_len) {
                                if (PRINTF_DESTLENGTH()) {
                                    PRINTF_PUSHCHAR(' ');
                                } else {
                                    truncated_due_to_space = TRUE;
                                }
                                element_len--;
                            }
                        }
                        if (short_prefix) {
                            while (*short_str != '\0' && element_len) {
                                if (PRINTF_DESTLENGTH()) {
#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(suppress: 6269)
#endif
#ifdef UNICODE
                                    PRINTF_PUSHCHAR(PRINTF_ANSI_TO_UNICODE(*short_str));
#else
                                    PRINTF_PUSHCHAR(*short_str);
#endif
                                } else {
                                    truncated_due_to_space = TRUE;
                                }
                                short_str++;
                                element_len--;
                            }
                        } else if (long_prefix) {
                            while (*long_str != '\0' && element_len) {
                                if (PRINTF_DESTLENGTH()) {
#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(suppress: 6269)
#endif
#ifdef UNICODE
                                    PRINTF_PUSHCHAR(*long_str);
#else
                                    PRINTF_PUSHCHAR(PRINTF_UNICODE_TO_ANSI(*long_str));
#endif
                                } else {
                                    truncated_due_to_space = TRUE;
                                }
                                long_str++;
                                element_len--;
                            }
                        }

                        if (leftalign) {
                            while (element_len--) {
                                if (PRINTF_DESTLENGTH()) {
                                    PRINTF_PUSHCHAR(' ');
                                } else {
                                    truncated_due_to_space = TRUE;
                                }
                            }
                        }
                    }
                    break;
                case 'y':
                    {
                        PYORI_STRING str = va_arg(marker, PYORI_STRING);
                        LPSTR  short_str = (LPSTR)str->StartOfString;
                        LPWSTR long_str = (LPWSTR)str->StartOfString;
                        DWORD str_offset;

                        if (!short_prefix && !long_prefix) {
                            long_prefix = TRUE;
                        }

                        if (element_len != (DWORD)-1 && !leftalign) {
                            DWORD str_len;
                            str_len = str->LengthInChars;

                            while (element_len > str_len) {
                                if (PRINTF_DESTLENGTH()) {
                                    PRINTF_PUSHCHAR(' ');
                                } else {
                                    truncated_due_to_space = TRUE;
                                }
                                
                                element_len--;
                            }
                        }
                        str_offset = 0;
                        if (short_prefix) {
                            while (str_offset < str->LengthInChars && element_len) {
                                if (PRINTF_DESTLENGTH()) {
#ifdef UNICODE
                                    PRINTF_PUSHCHAR(PRINTF_ANSI_TO_UNICODE(short_str[str_offset]));
#else
                                    PRINTF_PUSHCHAR(short_str[str_offset]);
#endif
                                } else {
                                    truncated_due_to_space = TRUE;
                                }
                                str_offset++;
                                element_len--;
                            }
                        } else if (long_prefix) {
                            while (str_offset < str->LengthInChars && element_len) {
                                if (PRINTF_DESTLENGTH()) {
#ifdef UNICODE
                                    PRINTF_PUSHCHAR(long_str[str_offset]);
#else
                                    PRINTF_PUSHCHAR(PRINTF_UNICODE_TO_ANSI(long_str[str_offset]));
#endif
                                } else {
                                    truncated_due_to_space = TRUE;
                                }
                                str_offset++;
                                element_len--;
                            }
                        }

                        if (leftalign) {
                            while (element_len--) {
                                if (PRINTF_DESTLENGTH()) {
                                    PRINTF_PUSHCHAR(' ');
                                } else {
                                    truncated_due_to_space = TRUE;
                                }
                            }
                        }
                    }
                    break;
                case 'u':
                case 'd':
                case 'i':
                case 'x':
                case 'p':
                    if (!longlong_prefix) {
                        DWORD num, tempnum;
                        DWORD divisor, digits;
                        DWORD radix = 10;

                        //
                        //  If we're %i we're base 10, if we're %x we're
                        //  base 16
                        //

                        if (szFmt[src_offset] == 'x' || szFmt[src_offset] == 'p') {
                            radix = 16;
                        }

                        num = va_arg(marker, int);

                        //
                        //  Count the number of digits we have in the user's
                        //  input.  Stop if we hit the format specifier.
                        //  Code below will preserve low order values.
                        //

                        divisor = 1;
                        digits = 1;
                        tempnum = num;
                        while (tempnum > radix - 1 && digits < element_len) {
                            divisor *= radix;
                            tempnum = num / divisor;
                            digits++;
                        }

                        //
                        //  If the field specifier is larger, pad it with
                        //  either a zero or space depending on the format
                        //  If the field specifier is too small, output more
                        //  characters than the field specifier specifies.
                        //

                        if (element_len != (DWORD)-1 && digits < element_len) {
                            tempnum = element_len - digits;
                            while (tempnum > 0) {
                                if (!PRINTF_DESTLENGTH()) {
                                    truncated_due_to_space = TRUE;
                                    break;
                                }
                                if (leadingzero) {
                                    PRINTF_PUSHCHAR('0');
                                } else {
                                    PRINTF_PUSHCHAR(' ');
                                }
                                tempnum--;
                            }
                        }

                        do {
                            tempnum = (num / divisor) % radix;

                            if (!PRINTF_DESTLENGTH()) {
                                truncated_due_to_space = TRUE;
                                break;
                            }

                            if (tempnum > 9) {
                                PRINTF_PUSHCHAR((UCHAR)(tempnum + 'a' - 10));
                            } else {
                                PRINTF_PUSHCHAR((UCHAR)(tempnum + '0'));
                            }

                            divisor /= radix;
                            digits--;

                        } while(digits > 0);

                    } else {
                        DWORDLONG num, tempnum;
                        DWORD digits;
                        DWORD digitcount;
                        DWORD radix = 10;

                        //
                        //  If we're %i we're base 10, if we're %x we're
                        //  base 16
                        //

                        if (szFmt[src_offset] == 'x' || szFmt[src_offset] == 'p') {
                            radix = 16;
                        }

                        num = va_arg(marker, DWORDLONG);

                        //
                        //  Count the number of digits we have in the user's
                        //  input.  Stop if we hit the format specifier.
                        //  Code below will preserve low order values.
                        //

                        digits = 1;
                        tempnum = num;
                        while (tempnum > radix - 1) {
                            tempnum = tempnum / radix;
                            digits++;
                        }

                        //
                        //  If the field specifier is larger, pad it with
                        //  either a zero or space depending on the format
                        //  If the field specifier is too small, output more
                        //  characters than the field specifier specifies.
                        //

                        if (element_len != (DWORD)-1 && digits < element_len) {
                            tempnum = element_len - digits;
                            while (tempnum > 0) {
                                if (!PRINTF_DESTLENGTH()) {
                                    truncated_due_to_space = TRUE;
                                    break;
                                }
                                if (leadingzero) {
                                    PRINTF_PUSHCHAR('0');
                                } else {
                                    PRINTF_PUSHCHAR(' ');
                                }
                                tempnum--;
                            }
                        }

                        do {
                            tempnum = num;
                            for (digitcount = 1; digitcount < digits; digitcount++) {
                                tempnum = tempnum / radix;
                            }
                            tempnum = tempnum % radix;

                            if (!PRINTF_DESTLENGTH()) {
                                truncated_due_to_space = TRUE;
                                break;
                            }

                            if (tempnum > 9) {
                                PRINTF_PUSHCHAR((UCHAR)(tempnum + 'a' - 10));
                            } else {
                                PRINTF_PUSHCHAR((UCHAR)(tempnum + '0'));
                            }

                            digits--;
                        } while (digits > 0);
                    }
                    break;
                default:
                    {
                        LPTSTR szErr = _T("FMTERR");
                        i = 0;
                        while (szErr[i] != '\0') {
                            if (PRINTF_DESTLENGTH()) {
                                PRINTF_PUSHCHAR(szErr[i++]);
                            } else {
                                truncated_due_to_space = TRUE;
                            }
                        }
                    }
                    break;
            }

            src_offset++;

        } else {
            if (PRINTF_DESTLENGTH()) {
                PRINTF_PUSHCHAR(szFmt[src_offset++]);
            } else {
                truncated_due_to_space = TRUE;
            }
        }

        if (truncated_due_to_space) {
            break;
        }
    }

#ifndef PRINTF_SIZEONLY
    if (dest_offset >= len || szFmt[src_offset] != '\0') {
        szDest[0] = '\0';
        return -1;
    }
#endif
    ASSERT(!truncated_due_to_space);
    PRINTF_PUSHCHAR('\0');

#ifndef PRINTF_SIZEONLY
    dest_offset--;
#endif

    return dest_offset;
}

// vim:sw=4:ts=4:et:
