section .text
 
; Принимает код возврата и завершает текущий процесс
exit: 
    mov rax, 60                        
    xor rdi, rdi 
    syscall 

; Принимает указатель на нуль-терминированную строку, возвращает её длину
string_length:
    xor rax, rax                    
    .loop:
        cmp byte[rdi+rax], 0           
        je .end
        inc rax
        jmp .loop
    .end:
        ret

; Принимает указатель на нуль-терминированную строку, выводит её в stdout
print_string:
    call string_length              ; return RAX
    mov rsi, rdi                    ; string adress             
    mov rdx, rax                    ; length
    mov rdi, 1                      ; descriptor          
    mov rax, 1                      ; № syscall           
    push rcx
    syscall
    pop rcx
    ret

; Принимает код символа и выводит его в stdout
print_char:
    push rdi
    mov rsi, rsp                    ; string adress          
    mov rdx, 1                      ; length               
    mov rdi, 1                      ; descriptor          
    mov rax, 1                      ; № syscall            
    push rcx
    syscall
    pop rcx
    pop rdi
    ret

; Переводит строку (выводит символ с кодом 0xA)
print_newline:
    mov rdi, 0xA                                            
    jmp print_char
    
; Выводит беззнаковое 8-байтовое число в десятичном формате 
; Совет: выделите место в стеке и храните там результаты деления
; Не забудьте перевести цифры в их ASCII коды.
print_uint:                       
    mov rax, rdi                       ; rax / r8 = rax (ost: rdx)
    mov r8, 10
    mov r9, rsp                                                     
    dec rsp                                                         
    mov byte[rsp], 0                   
    .loop:
        xor rdx, rdx                   
        div r8                        
        add dl, '0'
        dec rsp
        mov [rsp], dl                  ; dl = last digit (ostatok)
        test rax, rax
        jnz .loop
    mov rdi, rsp
    call print_string
    mov rsp, r9
    ret

; Выводит знаковое 8-байтовое число в десятичном формате 
print_int:
    mov rax, rdi
    cmp rax, 0                                                         
    jns print_uint                 ; if SF = 0 (positive)              
    push rdi
    mov rdi, '-'
    call print_char                
    pop rdi          
    neg rdi
    jmp print_uint 

; Читает один символ из stdin и возвращает его. Возвращает 0 если достигнут конец потока
read_char:
    dec rsp
    mov byte[rsp], 0                                                 
    mov rsi, rsp                    ; string adress             
    mov rdx, 1                      ; length
    mov rdi, 0                      ; descriptor
    mov rax, 0                      ; № syscall
    push rcx
    syscall
    pop rcx

    xor rax, rax
    mov al, byte[rsp]
    inc rsp
    ret

; Принимает: адрес начала буфера, размер буфера
; Читает в буфер слово из stdin, пропуская пробельные символы в начале, .
; Пробельные символы это пробел 0x20, табуляция 0x9 и перевод строки 0xA.
; Останавливается и возвращает 0 если слово слишком большое для буфера
; При успехе возвращает адрес буфера в rax, длину слова в rdx.
; При неудаче возвращает 0 в rax
; Эта функция должна дописывать к слову нуль-терминатор
read_word:                
    mov r8, rdi             ; buffer adress
    mov r9, rsi             ; buffer length
    xor r10, r10            ; counter 
    
    .A:                                          
        call read_char
        cmp al, 0x20
        je .A
        cmp al, 0x9
        je .A
        cmp al, 0xA
        je .A
        cmp al, 0
        je .END
        jmp .B
    .B:
        mov byte[r8+r10], al
        inc r10
        call read_char
        cmp al, 0x20
        je .END
        cmp al, 0x9
        je .END
        cmp al, 0xA
        je .END
        cmp al, 0
        je .END
        cmp r9, r10
        ja .B
        xor rax, rax
        ret

    .END: 
        mov rax, r8
        mov rdx, r10
        mov byte[r8+r10], 0
        ret

; Принимает указатель на строку, пытается                         ;; rax = num
; прочитать из её начала беззнаковое число                        ;; rdx = length (or 0)
; Возвращает в rax: число, rdx : его длину в символах             ;; abc -> rax = 0 rdx = 0
; rdx = 0 если число прочитать не удалось                         ;; 23 -> rax = 23, rdx = 2
parse_uint:
    xor rcx, rcx
    xor rax, rax

    .A:                             
        mov cl, [rdi]               
        inc rdi
        cmp cl, '0'
        jb .ERR
        cmp cl, '9'
        ja .ERR

        mov r8, 1                   ; length
        sub cl, '0'
        mov rax, rcx 

        jmp .B                      
    .B:                              
        mov cl, [rdi]         
        inc rdi
        cmp cl, '0'
        jb .OK
        cmp cl, '9'
        ja .OK

        inc r8
        mov r10, 10                             ; rax *= 10
        mul r10                                 ; rax += cl - '0'  
        sub cl, '0'
        add rax, rcx

        jmp .B
    .OK:
        mov rdx, r8
        ret   
    .ERR:
        xor rdx, rdx
        ret

; Принимает указатель на строку, пытается
; прочитать из её начала знаковое число.
; Если есть знак, пробелы между ним и числом не разрешены.
; Возвращает в rax: число, rdx : его длину в символах (включая знак, если он был) 
; rdx = 0 если число прочитать не удалось
parse_int:
    xor rcx, rcx
    xor rax, rax                      

    .A:
        mov cl, [rdi]               
        inc rdi
        mov r8, 1                   ; length
        cmp cl, '-'
        je .SIGN
        cmp cl, '+'
        je .B
                                    
        cmp cl, '0'
        jb .ERR
        cmp cl, '9'
        ja .ERR

        sub cl, '0'
        mov rax, rcx 

        jmp .C
    .SIGN:
        mov r9, 1
        jmp .B
    .B:                            
        mov cl, [rdi]               
        inc rdi
        cmp cl, '0'
        jb .ERR
        cmp cl, '9'
        ja .ERR

        inc r8                              ; length
        sub cl, '0'
        mov rax, rcx 

        jmp .C                      
    .C:                              
        mov cl, [rdi]         
        inc rdi
        cmp cl, '0'
        jb .OK
        cmp cl, '9'
        ja .OK

        inc r8
        mov r10, 10                             ; rax *= 10
        mul r10                                 ; rax += cl - '0'  
        sub cl, '0'
        add rax, rcx
        jmp .C
    .OK:
        mov rdx, r8
        cmp r9, 1
        je .OKK
        ret
    .OKK:
        neg rax
        ret
    .ERR:
        xor rdx, rdx
        ret

; Принимает два указателя на нуль-терминированные строки, возвращает 1 если они равны, 0 иначе
string_equals:
    xor r8, r8                             ; counter
    xor rcx, rcx
    xor rdx, rdx
    .loop:
        mov cl, [rdi+r8]
        mov dl, [rsi+r8]
        cmp cl, dl
        jne .ERR
        cmp byte[rdi+r8], 0
        je .OK
        inc r8
        jmp .loop
    .OK:
        mov rax, 1
        ret
    .ERR:
        xor rax, rax
        ret

; Принимает указатель на строку, указатель на буфер и длину буфера
; Копирует строку в буфер
; Возвращает длину строки если она умещается в буфер, иначе 0
string_copy:
    xor rcx, rcx                 ; rdi = addr,  rsi = buff, rdx = buff size
    xor r8, r8
    .loop:
        cmp r8, rdx 
        je .ERR
        mov cl, [rdi+r8]
        mov [rsi+r8], cl
        inc r8                   ; length
        cmp cl, 0
        je .OK
        jmp .loop
    .OK:
        mov rax, r8
        ret
    .ERR:
        xor rax, rax
        ret