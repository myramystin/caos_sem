.section .data
base_array: .quad 100, 200, 300, 400, 500, 600, 700, 800

.section .text
.global main

main:
    leaq base_array(%rip), %rbx     # Базовый адрес
    movq $2, %rcx                   # Индекс
    movq $10, %rdx                  # Смещение
    
    
    # 1. Только база: [base]
    movq (%rbx), %rax               # base_array[0]
    
    # 2. База + смещение: [base + disp]
    movq 16(%rbx), %rax             # base_array[2] (16 = 2 * 8 байт)
    
    # 3. База + индекс: [base + index]
    movq (%rbx, %rcx), %rax         # НЕ используйте без scale!
    
    # 4. База + индекс * масштаб: [base + index * scale]
    movq (%rbx, %rcx, 8), %rax      # base_array[index] правильно
    
    # 5. База + смещение + индекс * масштаб: [base + disp + index * scale]
    movq 8(%rbx, %rcx, 8), %rax     # base_array[1 + index]
    
    # 6. Использование двух регистров как базы и индекса
    movq %rbx, %rsi                 # Копируем базу
    addq $24, %rsi                  # Сдвигаем базу на 3 элемента
    movq (%rsi, %rcx, 8), %rax      # (base + 24) + index * 8
    
    # 7. Сложные вычисления с LEA
    leaq (%rbx, %rcx, 8), %rdi      # Адрес base_array[index]
    leaq 32(%rbx, %rcx, 4), %rsi    # base + 32 + index * 4
    
    movq $60, %rax
    xorq %rdi, %rdi
    syscall