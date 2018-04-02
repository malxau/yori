;
; BIGMUL.ASM
;
; Implementation for for signed and unsigned multiplication of a 64 bit integer
; by a 64 bit integer.
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
; _allmul(
;     LARGE_INTEGER Value1, [High ESP + 8, Low ESP + 4],
;     LARGE_INTEGER Value2 [High ESP + 16, Low ESP + 12]
; );

; Multiply low by high of both components, adding them into a temporary
; register.  Multiply low by low, which might result in a high component,
; and add back the temporary result into this high component.

_allmul proc

push ebx

mov eax, [esp + 8]
mov edx, [esp + 20]
mul edx
mov ebx, eax

mov eax, [esp + 16]
mov edx, [esp + 12]
mul edx
add ebx, eax

mov eax, [esp + 8]
mov edx, [esp + 16]
mul edx
add edx, ebx

pop ebx

ret 16
_allmul endp

END
