.section .rodata
    format_str: .ascii "Processing data\n"
    format_len = . - format_str
    numbers: .quad 10, 20, 30, 40, 50

.section .data
    result: .quad 0

.section .text
.global _start

_start:
    # Загружаем адрес массива (RIP-relative)
    leaq numbers(%rip), %rsi
    
    # Загружаем первый элемент
    movq numbers(%rip), %rax        # numbers[0] = 10
    
    # Загружаем второй элемент
    movq numbers+8(%rip), %rbx      # numbers[1] = 20
    
    # Складываем их
    addq %rbx, %rax                 # rax = 10 + 20 = 30
    
    # Сохраняем результат
    movq %rax, result(%rip)
    
    # Выход
    movq $60, %rax
    movq result(%rip), %rdi
    syscall
