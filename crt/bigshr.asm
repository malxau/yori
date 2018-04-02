;
; BIGSHR.ASM
;
; Implementation for for signed and unsigned right shift of a 64 bit integer.
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

; ULARGE_INTEGER [High EDX, Low EAX]
; _aullshr(
;     ULARGE_INTEGER Value, [High EDX, Low EAX]
;     UCHAR Shift [CL]
; );

public _aullshr
_aullshr proc

; If the shift is for more than 64 bits, all the data would be gone, so
; return zero.  If the shift is for more than 32 bits, the high 32 bits
; of the result must be zero, and the low 32 bits contains the high 32
; bits of input after shifting.  If the shift is less than 32 bits, then
; both components must be shifted with bits carried between the two.

cmp cl,64
jae aullshr_no_shift
cmp cl,32
jae aullshr_long_shift
jmp aullshr_short_shift

aullshr_short_shift:
shrd eax, edx, cl
shr edx, cl
ret

aullshr_long_shift:
sub cl, 32
shr edx, cl
mov eax, edx
xor edx, edx
ret

aullshr_no_shift:
xor eax, eax
xor edx, edx
ret

_aullshr endp

; LARGE_INTEGER [High EDX, Low EAX]
; _allshr(
;     LARGE_INTEGER Value, [High EDX, Low EAX]
;     UCHAR Shift [CL]
; );

; It's not clear to me what the meaning of a signed bitshift is, but for now
; give it to the unsigned implementation.

public _allshr
_allshr proc
jmp _aullshr
_allshr endp

END
