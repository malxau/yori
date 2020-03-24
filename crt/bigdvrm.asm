;
; BIGDVRM.ASM
;
; Implementation for for signed and unsigned division of a 64 bit integer
; by a 32 bit integer, returning the result and remainder.
;
; Copyright (c) 2017-2020 Malcolm J. Smith
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

externdef _aulldiv:near
externdef _aullrem:near
externdef _alldiv:near
externdef _allrem:near

; LARGE_INTEGER Result [High EDX, Low EAX]
; LARGE_INTEGER Remainder [High EBX, Low ECX]
; _aulldvrm(
;     LARGE_INTEGER Dividend, [High ESP + 8, Low ESP + 4],
;     LARGE_INTEGER Divisor [High ESP + 16, Low ESP + 12]
; );

public _aulldvrm
_aulldvrm proc

; Push the arguments for the next function

mov edx, [esp + 16]
mov eax, [esp + 12]
push edx
push eax

mov edx, [esp + 16]
mov eax, [esp + 12]
push edx
push eax

; Get the remainder

call _aullrem

; Save the remainder into registers that will be preserved

push esi
mov ebx, edx
mov esi, eax

; Push the arguments for the next function

mov edx, [esp + 20]
mov eax, [esp + 16]
push edx
push eax

mov edx, [esp + 20]
mov eax, [esp + 16]
push edx
push eax

; Get the result

call _aulldiv

mov ecx, esi
pop esi

ret 16

_aulldvrm endp

; LARGE_INTEGER Result [High EDX, Low EAX]
; LARGE_INTEGER Remainder [High EBX, Low ECX]
; _alldvrm(
;     LARGE_INTEGER Dividend, [High ESP + 8, Low ESP + 4],
;     LARGE_INTEGER Divisor [High ESP + 16, Low ESP + 12]
; );

public _alldvrm
_alldvrm proc

; Push the arguments for the next function

mov edx, [esp + 16]
mov eax, [esp + 12]
push edx
push eax

mov edx, [esp + 16]
mov eax, [esp + 12]
push edx
push eax

; Get the remainder

call _allrem

; Save the remainder into registers that will be preserved

push esi
mov ebx, edx
mov esi, eax

; Push the arguments for the next function

mov edx, [esp + 20]
mov eax, [esp + 16]
push edx
push eax

mov edx, [esp + 20]
mov eax, [esp + 16]
push edx
push eax

; Get the result

call _alldiv

mov ecx, esi
pop esi

ret 16

_alldvrm endp

END
