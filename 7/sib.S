.section .data
# Массивы различных типов
byte_array:  .byte 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80
word_array:  .word 100, 200, 300, 400, 500
dword_array: .long 1000, 2000, 3000, 4000
qword_array: .quad 10000, 20000, 30000, 40000

# Двумерный массив 3x4 (int matrix[3][4])
matrix: .long 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12

.section .text
.global main

main:    
    # 1. Доступ к byte_array[5]
    leaq byte_array(%rip), %rbx     # Загружаем базовый адрес
    movq $5, %rcx                   # Индекс элемента
    movb (%rbx, %rcx, 1), %al       # byte_array[5] -> %al (scale=1 для байт)
    
    # 2. Доступ к word_array[3] (каждый элемент 2 байта)
    leaq word_array(%rip), %rbx
    movq $3, %rcx
    movw (%rbx, %rcx, 2), %dx       # word_array[3] -> %dx (scale=2 для word)
    
    # 3. Доступ к dword_array[2] (каждый элемент 4 байта)
    leaq dword_array(%rip), %rbx
    movq $2, %rcx
    movl (%rbx, %rcx, 4), %esi      # dword_array[2] -> %esi (scale=4 для dword)
    
    # 4. Доступ к qword_array[1] (каждый элемент 8 байт)
    leaq qword_array(%rip), %rbx
    movq $1, %rcx
    movq (%rbx, %rcx, 8), %rdi      # qword_array[1] -> %rdi (scale=8 для qword)
    
    # Доступ к matrix[1][2] (второй ряд, третий столбец)
    leaq matrix(%rip), %rbx         # Базовый адрес матрицы
    movq $1, %rcx                   # row = 1
    movq $2, %rdx                   # col = 2
    
    # Вычисляем смещение: row * 4 * sizeof(int) + col * sizeof(int)
    # = 1 * 4 * 4 + 2 * 4 = 16 + 8 = 24 байта от начала
    imulq $16, %rcx                 # row * (4 columns * 4 bytes)
    movl (%rbx, %rcx, 1), %r8d      # Базовое смещение по строке
    movl (%rbx, %rdx, 4), %r9d      # matrix[0][col]
    
    # Более элегантный способ - использование LEA для вычисления адреса
    movq $1, %rcx                   # row
    movq $2, %rdx                   # col
    leaq (%rcx, %rcx, 2), %rax      # rax = row * 3
    leaq (%rax, %rax), %rax         # rax = row * 6  
    addq %rdx, %rax                 # rax = row * 6 + col (неправильно для нашего случая)
    
    # Правильный расчет для matrix[3][4]:
    shlq $2, %rcx                   # row * 4 (количество столбцов)
    addq %rdx, %rcx                 # row * 4 + col (линейный индекс)
    movl (%rbx, %rcx, 4), %r10d     # matrix[1][2] -> %r10d
    
    movq $60, %rax
    movq $0, %rdi 
    syscall
