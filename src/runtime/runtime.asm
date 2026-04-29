; runtime.asm - Минимальная рантайм-библиотека для компилятора
; Следует System V AMD64 ABI и Linux x86-64 системным вызовам

DEFAULT REL

section .data
    ; Буфер для преобразования числа в строку
    print_buffer times 32 db 0
    print_buffer_end db 10  ; перевод строки
    
    ; Буфер для чтения числа
    read_buffer times 32 db 0
    
section .text
    global print_int
    global print_string
    global read_int
    global exit
    global _start
    
    extern main

; print_int - вывод целого числа в stdout
; Аргумент: rdi - число для вывода
print_int:
    push rbp
    mov rbp, rsp
    
    ; Сохраняем число
    mov rax, rdi
    
    ; Проверяем на отрицательное
    test rax, rax
    jge .convert
    
    ; Выводим минус
    push rax
    mov rax, 1          ; syscall write
    mov rdi, 1          ; stdout
    lea rsi, [minus_sign]
    mov rdx, 1          ; длина 1
    syscall
    pop rax
    neg rax             ; делаем положительным
    
.convert:
    ; Преобразуем число в строку (в обратном порядке)
    mov rdi, print_buffer_end - 2  ; точка перед \n
    mov rcx, 0          ; счетчик цифр
    
.convert_loop:
    xor rdx, rdx
    mov rbx, 10
    div rbx             ; rax / 10, остаток в rdx
    
    add dl, '0'         ; преобразуем в ASCII
    mov [rdi], dl
    dec rdi
    inc rcx
    
    test rax, rax
    jnz .convert_loop
    
    ; Выводим результат
    ; rdi указывает на начало строки (-1)
    lea rsi, [rdi + 1]  ; начало строки
    mov rdx, rcx        ; длина
    add rdx, 1          ; + перевод строки
    
    ; Добавляем перевод строки
    mov byte [print_buffer_end - 1], 10
    
    ; Системный вызов write
    mov rax, 1          ; syscall write
    mov rdi, 1          ; stdout
    syscall
    
    pop rbp
    ret

; print_string - вывод строки в stdout
; Аргумент: rdi - указатель на null-terminated строку
print_string:
    push rbp
    mov rbp, rsp
    
    ; Вычисляем длину строки
    mov rsi, rdi        ; сохраняем указатель для syscall
    mov rdx, 0          ; счетчик
    
.strlen:
    cmp byte [rdi], 0
    je .do_write
    inc rdi
    inc rdx
    jmp .strlen
    
.do_write:
    ; Системный вызов write
    mov rax, 1          ; syscall write
    mov rdi, 1          ; stdout
    syscall
    
    pop rbp
    ret

; read_int - чтение целого числа из stdin
; Возвращает: rax - прочитанное число
read_int:
    push rbp
    mov rbp, rsp
    
    ; Читаем строку из stdin
    mov rax, 0          ; syscall read
    mov rdi, 0          ; stdin
    lea rsi, [read_buffer]
    mov rdx, 32         ; макс. длина
    syscall
    
    ; rax содержит количество прочитанных байт
    cmp rax, 0
    jle .return_zero
    
    ; Преобразуем строку в число
    xor rax, rax         ; результат = 0
    xor rcx, rcx         ; счетчик
    lea rsi, [read_buffer] ; указатель на буфер
    xor rbx, rbx         ; знак положительный (0)
    
    ; Проверяем на минус
    cmp byte [rsi], '-'
    jne .parse_loop
    inc rsi
    mov rbx, 1           ; знак отрицательный
    
.parse_loop:
    movzx rdx, byte [rsi + rcx]
    
    ; Проверяем конец строки
    cmp dl, 10           ; \n
    je .done
    cmp dl, 0            ; null
    je .done
    cmp dl, ' '          ; пробел
    je .done
    
    ; Проверяем, что это цифра
    cmp dl, '0'
    jl .done
    cmp dl, '9'
    jg .done
    
    sub dl, '0'          ; преобразуем в число
    imul rax, 10
    add rax, rdx
    
    inc rcx
    cmp rcx, 10          ; максимум 10 цифр
    jl .parse_loop
    
.done:
    ; Применяем знак
    test rbx, rbx
    jz .return
    neg rax
    
.return:
    pop rbp
    ret
    
.return_zero:
    xor rax, rax
    pop rbp
    ret

; exit - завершение программы
; Аргумент: rdi - код возврата
exit:
    mov rax, 60         ; syscall exit
    syscall
    ; Не возвращается

; _start - точка входа в программу
_start:
    ; Вызываем main
    call main
    
    ; Возвращаемое значение из main в rdi
    mov rdi, rax
    
    ; Завершаем программу
    call exit

section .data
    minus_sign db '-'