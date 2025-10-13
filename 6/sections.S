.section .data
    greeting: .ascii "Hello, World!\n"
    greeting_len = . - greeting
    number: .quad 42

.section .bss
    .lcomm input_buffer, 256    # резервируем 256 байт

.section .text
.global _start

_start:
    # Вывод строки
    movq $1, %rax               # sys_write
    movq $1, %rdi               # stdout
    movq $greeting, %rsi        # адрес строки
    movq $greeting_len, %rdx    # длина строки
    syscall
    
    # Загружаем число из .data секции
    movq number(%rbx), %rax
    
    # Выход с этим числом как кодом возврата
    movq $60, %rax
    movq $0, %rdi             
    movq number(%rdi), %rdi
    syscall
