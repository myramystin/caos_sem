.section .data

# Моделируем C структуру:
# struct Person {
#     char initial;        // 1 байт
#     int age;            // 4 байта  
#     char grade;         // 1 байт
#     long salary;        // 8 байт
# };

# Версия БЕЗ учета выравнивания (неправильно)
person_bad:
    .byte 'J'               # initial - 1 байт
    .long 25                # age - 4 байта (невыровнен!)
    .byte 'A'               # grade - 1 байт  
    .quad 50000             # salary - 8 байт (невыровнен!)

# Размер: 1 + 4 + 1 + 8 = 14 байт

# Версия С учетом выравнивания (правильно)
person_good:
    .byte 'J'               # initial - 1 байт на адресе X
    .align 4                # Padding до кратного 4
    .long 25                # age - 4 байта на адресе X+4 (выровнен ✓)  
    .byte 'A'               # grade - 1 байт на адресе X+8
    .align 8                # Padding до кратного 8
    .quad 50000             # salary - 8 байт на адресе X+16 (выровнен ✓)

# Реальный размер: 1 + 3(padding) + 4 + 1 + 7(padding) + 8 = 24 байта

people_array:
person1:
    .byte 'A'               # 1 байт
    .space 3                # 3 байта padding
    .long 30                # 4 байта (выровнено)
    .byte 'B'               # 1 байт  
    .space 7                # 7 байт padding
    .quad 60000             # 8 байт (выровнено)
    
person2:  
    .byte 'C'               # Начало второй структуры
    .space 3                # padding
    .long 35                # 4 байта
    .byte 'A'               # 1 байт
    .space 7                # padding  
    .quad 70000             # 8 байт

.section .text
.global main

main:
    leaq person_good(%rip), %rbx
    
    # Читаем поля с правильными смещениями
    movb 0(%rbx), %al       # initial на смещении 0
    movl 4(%rbx), %ecx      # age на смещении 4 (после padding)
    movb 8(%rbx), %dl       # grade на смещении 8  
    movq 16(%rbx), %rsi     # salary на смещении 16 (после padding)
    
    # Работа с массивом структур
    leaq people_array(%rip), %rdi
    
    # Доступ к person1.age
    movl 4(%rdi), %r8d      # person1 + смещение age
    
    # Доступ к person2.age (person2 начинается через 24 байта)
    movl 28(%rdi), %r9d     # person1 + 24 + смещение age
    
    # Завершение  
    movq $60, %rax
    xorq %rdi, %rdi
    syscall
