;
; BIGDIV.ASM
;
; Implementation for for signed and unsigned division of a 64 bit integer
; by a 32 bit integer.
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
; _aulldiv(
;     LARGE_INTEGER Dividend, [High ESP + 8, Low ESP + 4],
;     LARGE_INTEGER Divisor [High ESP + 16, Low ESP + 12]
; );

public _aulldiv
_aulldiv proc

; This implementation doesn't support 64 bit divisors.  If one is specified,
; fail.

mov eax, [esp + 16]
test eax, eax
jnz aulldiv_overflow

push ebx

; Divide the high 32 bits by the low 32 bits, save it in ebx,
; and leave the remainder in edx.  Then, divide the low 32 bits
; plus the remainder, which must fit in a 32 bit value.  To satisfy
; the calling convention, move ebx to edx, and return.

mov ecx, [esp + 16]
xor edx, edx
mov eax, [esp + 12]
div ecx
mov ebx, eax
mov eax, [esp + 8]
div ecx
mov edx, ebx
pop ebx
ret 16

aulldiv_overflow:
int 3
xor edx, edx
xor eax, eax
ret 16

_aulldiv endp

; LARGE_INTEGER [High EDX, Low EAX]
; _alldiv(
;     LARGE_INTEGER Dividend, [High ESP + 8, Low ESP + 4],
;     LARGE_INTEGER Divisor [High ESP + 16, Low ESP + 12]
; );

public _alldiv
_alldiv proc

push ebx
xor ebx,ebx

; Check if the divisor is positive or negative.  If negative, increment
; ebx to indicate another negative number was found, and convert it to
; positive

alldiv_test_divisor:
mov edx, [esp + 20]
mov eax, [esp + 16]
bt edx, 31
jnc alldiv_positive_divisor

inc ebx
neg edx
neg eax
sbb edx, 0

; Push the now positive divisor onto the stack, thus moving esp

alldiv_positive_divisor:
push edx
push eax

; Check if the dividend is positive or negative.  If negative, increment
; ebx to indicate another negative number was found, and convert it to
; positive

mov edx, [esp + 20]
mov eax, [esp + 16]
bt edx, 31
jnc alldiv_positive_dividend

inc ebx
neg edx
neg eax
sbb edx, 0

; Push the now positive dividend onto the stack, thus moving esp

alldiv_positive_dividend:
push edx
push eax

; Call the positive version of this routine

call _aulldiv

; Test if an odd number of negative numbers were found.  If so, convert the
; result back to negative, otherwise return the result as is.

bt ebx, 0
jnc alldiv_return_positive

neg edx
neg eax
sbb edx, 0

alldiv_return_positive:
pop ebx
ret 16
_alldiv endp


END
