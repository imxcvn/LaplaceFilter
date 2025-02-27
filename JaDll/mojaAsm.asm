.DATA

.CODE

ThreadStartProc proc

    push rax 
    push rbx 
    push rcx 
    push rdx 
    push rsi 
    push rdi   
    push r8 
    push r9 
    push r10 
    push r11 
    push r12 
    push r13 
    push r14  

    ; Przenoszenie parametrow do rejestrow
    mov rsi, rcx       ; rsi - wskaznik na tablice wejsciowa (B, G, R) elementy typu unsigned char
    mov rdi, rdx       ; rdi - wskaznik na tablice wyjsciowa (B, G, R)
    mov r13, r8        ; r13 - szerokosc bitmapy w pikselach
    mov r14, r9        ; r14 - wysokosc bitmapy w pikselach  

; obliczenie indeksu bierzacego piksela
mov rdx, r13  ; rdx = width
add rdx, 1  ; rdx = width * y + x  
imul rdx, 3  ; rdx = (width * y + x) * 3
sub rdx, 6

mov r11, r13
imul r11, 3 ; r11 = szerokosc w bajtach

xor r9, r9   ; r9 = y

YLoop:

inc r9   ; y = 1
mov rax, r14  ; rax = height
dec rax   ; rax = height - 1
cmp r9, rax  ; jezeli r9 = rax to koniec
je Done

xor r10, r10  
mov r10, 3   ; x = 3
add rdx, 6
jmp XLoop

XLoop:

mov rax, r11  ; r11 = width (w bajtach)
sub rax, 18   ; width*3 - 18 (16 + 3 - 1)
cmp r10, rax  ; czy r10 wieksze lub równe od pierwszego "nielegalnego" elementu w wierszu
jge RightEdge  ; jezeli jest juz dalej lub równy niz pierwszy nielegalny el. do skacze to konca

; indeks poczatku
mov rcx, rdx
sub rcx, r11
sub rcx, 3

; wczytanie lewej wartoœci górny rz¹d
movdqu xmm1, [rsi + rcx]

; wczytanie œrodkowych wartoœci górnego rzêdu
movdqu xmm2, [rsi + rcx + 3]

; wczytanie prawych wartoœci górnego rzêdu
movdqu xmm3, [rsi + rcx + 6]

; przesuniêcie do pocz¹tku w nastêpnym rzêdzie
add rcx, r11

; wczytanie lewych wartosci œrodkowego rzêdu
movdqu xmm4, [rsi + rcx]

; wczytanie œrodkowych wartosci œrodkowego rzêdu
movdqu xmm5, [rsi + rcx + 3]

; wczytanie prawych wartoœci œrodkowego rzêdu
movdqu xmm6, [rsi + rcx + 6]

; przesuniêcie do pocz¹tku w nastêpnym rzêdzie
add rcx, r11

; wczytanie lewych wartosci dolnego rzêdu
movdqu xmm7, [rsi + rcx]

; wczytanie œrodkowych wartoœci dolnego rzêdu
movdqu xmm8, [rsi + rcx + 3]

; wczytanie prawych wartoœci dolnego rzêdu
movdqu xmm9, [rsi + rcx + 6]

; konwersja wartoœci w rejestrach do 16 bitowych

vpmovzxbw ymm1, xmm1  ; wstawienie zer do wy¿szych 8 bitów - rozszerzenie do 16-bit
vpmovzxbw ymm2, xmm2
vpmovzxbw ymm3, xmm3
vpmovzxbw ymm4, xmm4
vpmovzxbw ymm5, xmm5
vpmovzxbw ymm6, xmm6
vpmovzxbw ymm7, xmm7 
vpmovzxbw ymm8, xmm8
vpmovzxbw ymm9, xmm9

; pomno¿enie wartoœci w rejestrach przez odpowiednie elementy maski
    ; laplaceMask = {1, -2, 1, -2, 4, -2, 1, -2, 1}

    ; pomno¿enie wartoœci w rejestrach xmm2, xmm4, xmm6, xmm8 przez 2 i w xmm5 przez 4
    vpsllw ymm2, ymm2, 1
    vpsllw ymm4, ymm4, 1
    vpsllw ymm6, ymm6, 1
    vpsllw ymm8, ymm8, 1
    vpsllw ymm5, ymm5, 2
    
    ; dodawanie i odejmowanie rejestrów
    vpsubw ymm1, ymm1, ymm2
    vpsubw ymm3, ymm3, ymm4
    vpsubw ymm5, ymm5, ymm6
    vpsubw ymm7, ymm7, ymm8
    vpaddw ymm1, ymm1, ymm3
    vpaddw ymm5, ymm5, ymm7
    vpaddw ymm4, ymm1, ymm5
    vpaddw ymm0, ymm9, ymm4  

    ; konwersja na 8 bitowe dane
    vextracti128 xmm1, ymm0, 0
    vextracti128 xmm2, ymm0, 1
    vpackuswb xmm0, xmm1, xmm2 

; wczytanie wyniku od indeksu rdx
movdqu [rdi + rdx], xmm0

add rdx, 16 ; przesuniecie do nastepnego el. start
add r10, 16 ; odpowiednie przesuniecie licznika w wierszu
jmp XLoop

RightEdge:
mov r8, r11
sub r8, r10
sub r8, 3
cmp r8, 0
je YLoop

; Pojedyncze przetwarzanie
ProcessPixel:
cmp r8, 0
je YLoop

mov rcx, rdx
sub rcx, r11
sub rcx, 3

; wczytywanie
xor rax, rax
mov al, byte ptr [rsi + rcx]
vmovd xmm1, eax

mov al, byte ptr [rsi + rcx + 3]
vmovd xmm2, eax

mov al, byte ptr [rsi + rcx + 6]
vmovd xmm3, eax

add rcx, r11

mov al, byte ptr [rsi + rcx]
vmovd xmm4, eax

mov al, byte ptr [rsi + rcx + 3]
vmovd xmm5, eax

mov al, byte ptr [rsi + rcx + 6]
vmovd xmm6, eax

add rcx, r11

mov al, byte ptr [rsi + rcx]
vmovd xmm7, eax

mov al, byte ptr [rsi + rcx + 3]
vmovd xmm8, eax

mov al, byte ptr [rsi + rcx + 6]
vmovd xmm9, eax

vpsllw ymm2, ymm2, 1
vpsllw ymm4, ymm4, 1
vpsllw ymm6, ymm6, 1
vpsllw ymm8, ymm8, 1
vpsllw ymm5, ymm5, 2

vpsubw ymm1, ymm1, ymm2
vpsubw ymm3, ymm3, ymm4
vpsubw ymm5, ymm5, ymm6
vpsubw ymm7, ymm7, ymm8
vpaddw ymm1, ymm1, ymm3
vpaddw ymm5, ymm5, ymm7
vpaddw ymm4, ymm1, ymm5
vpaddw ymm0, ymm9, ymm4

vpackuswb xmm0, xmm0, xmm0

vmovd rax, xmm0

; wczytanie wyniku od indeksu rdx
mov byte ptr [rdi + rdx], al

inc rdx
dec r8
jmp ProcessPixel

    Done:

    pop r14        
    pop r13        
    pop r12        
    pop r11        
    pop r10        
    pop r9         
    pop r8         
    pop rdi        
    pop rsi        
    pop rdx        
    pop rcx        
    pop rbx        
    pop rax        

    ret  
    
ThreadStartProc endp
END                          