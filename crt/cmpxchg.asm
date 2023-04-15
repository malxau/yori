;
; CMPXCHG.ASM
;
; Implementation for InterlockedCompareExchange for compilers without it.
;
; Copyright (c) 2023 Malcolm J. Smith
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

.486
.MODEL FLAT, C
.CODE

; DWORD [EAX]
; _lock_cmpxchg(
;     PDWORD Destination,
;     DWORD Exchange,
;     DWORD Comperand
; );

public lock_cmpxchg@12
lock_cmpxchg@12 proc

mov eax,dword ptr [esp+0Ch]
mov ecx,dword ptr [esp+8h]
mov edx,dword ptr [esp+4]
lock cmpxchg dword ptr [edx],ecx
ret 0Ch

lock_cmpxchg@12 endp

END
