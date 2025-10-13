.section .rodata
    format_str: .ascii "Processing data\n"
    format_len = . - format_str
    numbers: .quad 10, 20, 30, 40, 50

.section .data
    result: .quad 0

.section .text
.global _start

_start:
    # Загружаем адрес массива (абсолютная адресация)
    movq $numbers, %rsi
    
    # Загружаем первый элемент
    movq $numbers, %rcx
    movq (%rcx), %rax               # numbers[0] = 10
    
    # Загружаем второй элемент
    movq $numbers, %rcx
    addq $8, %rcx                   # указатель на второй элемент
    movq (%rcx), %rbx               # numbers[1] = 20
    
    # Складываем их
    addq %rbx, %rax                 # rax = 10 + 20 = 30
    
    # Сохраняем результат
    movq $result, %rcx
    movq %rax, (%rcx)
    
    # Выход
    movq $60, %rax
    movq $result, %rcx
    movq (%rcx), %rdi
    syscall
