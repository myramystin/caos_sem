.section .data
# Выравниваем данные по 16-байтной границе (требование SSE)
.align 16
vector_a: .float 1.5, 2.5, 3.5, 4.5
vector_b: .float 0.5, 1.0, 1.5, 2.0
result:   .space 16                     # Место для результата (4 × 4 байта)

# Форматы для вывода
format_input: .asciz "Vector A: %.1f %.1f %.1f %.1f\n"
format_input2: .asciz "Vector B: %.1f %.1f %.1f %.1f\n"
format_result: .asciz "Result:   %.1f %.1f %.1f %.1f\n"

.section .text
.global main
.extern printf

main:
    pushq %rbp
    movq %rsp, %rbp
    
    # Выводим исходные векторы
    leaq format_input(%rip), %rdi
    # Для printf нужно преобразовать float в double
    cvtss2sd vector_a(%rip), %xmm0      # vector_a[0] -> double
    cvtss2sd vector_a+4(%rip), %xmm1    # vector_a[1] -> double  
    cvtss2sd vector_a+8(%rip), %xmm2    # vector_a[2] -> double
    cvtss2sd vector_a+12(%rip), %xmm3   # vector_a[3] -> double
    movq $4, %rax                        # 4 double аргумента
    call printf
    
    leaq format_input2(%rip), %rdi
    cvtss2sd vector_b(%rip), %xmm0
    cvtss2sd vector_b+4(%rip), %xmm1
    cvtss2sd vector_b+8(%rip), %xmm2
    cvtss2sd vector_b+12(%rip), %xmm3
    movq $4, %rax
    call printf
    
    # === ВЕКТОРНАЯ ОПЕРАЦИЯ ===
    # Загружаем векторы в SIMD регистры
    movaps vector_a(%rip), %xmm0        # Загрузить 4 float из vector_a
    movaps vector_b(%rip), %xmm1        # Загрузить 4 float из vector_b
    
    # Векторное сложение - ОДНА инструкция складывает 4 числа!
    addps %xmm1, %xmm0                 # %xmm0 = %xmm0 + %xmm1 (поэлементно)
    
    # Сохраняем результат
    movaps %xmm0, result(%rip)          # Сохранить 4 результата
    
    # Выводим результат
    leaq format_result(%rip), %rdi
    cvtss2sd result(%rip), %xmm0
    cvtss2sd result+4(%rip), %xmm1
    cvtss2sd result+8(%rip), %xmm2
    cvtss2sd result+12(%rip), %xmm3
    movq $4, %rax
    call printf
    
    movq $0, %rax
    popq %rbp
    ret
