;
; BIGSHL.ASM
;
; Implementation for for signed and unsigned left shift of a 64 bit integer.
;
; Copyright (c) 2017 Malcolm J. Smith
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
; THE SOFTWARE.
;

.386
.MODEL FLAT, C
.CODE

; LARGE_INTEGER [High EDX, Low EAX]
; _allshl(
;     LARGE_INTEGER Value, [High EDX, Low EAX]
;     UCHAR Shift [CL]
; );

public _allshl
_allshl proc

; If the shift is for more than 64 bits, all the data would be gone, so
; return zero.  If the shift is for more than 32 bits, the low 32 bits
; of the result must be zero, and the high 32 bits contains the low 32
; bits of input after shifting.  If the shift is less than 32 bits, then
; both components must be shifted with bits carried between the two.

cmp cl,64
jae allshl_no_shift
cmp cl,32
jae allshl_long_shift
jmp allshl_short_shift

allshl_short_shift:
shld edx, eax, cl
shl eax, cl
ret

allshl_long_shift:
sub cl, 32
shl eax, cl
mov edx, eax
xor eax, eax
ret

allshl_no_shift:
xor eax, eax
xor edx, edx
ret

_allshl endp

END
