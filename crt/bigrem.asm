;
; BIGREM.ASM
;
; Implementation for for signed and unsigned division of a 64 bit integer
; by a 32 bit integer, returning the remainder (ie., mod.)
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
; _aullrem(
;     LARGE_INTEGER Dividend, [High ESP + 8, Low ESP + 4],
;     LARGE_INTEGER Divisor [High ESP + 16, Low ESP + 12]
; );

public _aullrem
_aullrem proc

; This implementation doesn't support 64 bit divisors.  If one is specified,
; fail.

mov eax, [esp + 16]
test eax, eax 
jnz aullrem_overflow

; Divide the high 32 bits by the low 32 bits and leave the
; remainder in edx.  Then, divide the low 32 bits plus the
; remainder, which must fit in a 32 bit value, putting the
; remainder into edx.  To satisfy the calling convention,
; move this to eax and clear edx, and return.

mov ecx, [esp + 12]
xor edx, edx
mov eax, [esp + 8]
div ecx
mov eax, [esp + 4]
div ecx
mov eax, edx
xor edx, edx
ret 16

aullrem_overflow:
int 3
xor edx, edx
xor eax, eax
ret 16

_aullrem endp

; LARGE_INTEGER [High EDX, Low EAX]
; _allrem(
;     LARGE_INTEGER Dividend, [High ESP + 8, Low ESP + 4],
;     LARGE_INTEGER Divisor [High ESP + 16, Low ESP + 12]
; );

public _allrem
_allrem proc

push ebx
xor ebx,ebx

; Check if the divisor is positive or negative.  If negative, increment
; ebx to indicate another negative number was found, and convert it to
; positive

allrem_test_divisor:
mov edx, [esp + 20]
mov eax, [esp + 16]
bt edx, 31
jnc allrem_positive_divisor

inc ebx
neg edx
neg eax
sbb edx, 0

; Push the now positive divisor onto the stack, thus moving esp

allrem_positive_divisor:
push edx
push eax

; Check if the dividend is positive or negative.  If negative, increment
; ebx to indicate another negative number was found, and convert it to
; positive

mov edx, [esp + 20]
mov eax, [esp + 16]
bt edx, 31
jnc allrem_positive_dividend

inc ebx
neg edx
neg eax
sbb edx, 0

; Push the now positive dividend onto the stack, thus moving esp

allrem_positive_dividend:
push edx
push eax

; Call the positive version of this routine

call _aullrem

; Test if an odd number of negative numbers were found.  If so, convert the
; result back to negative, otherwise return the result as is.

bt ebx, 0
jnc allrem_return_positive

neg edx
neg eax
sbb edx, 0

allrem_return_positive:
pop ebx
ret 16

_allrem endp

END
