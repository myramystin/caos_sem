.section .text
.global main

main:
    
    # === Флаг нуля (ZF) ===
    movq $5, %rax
    subq $5, %rax                   # 5 - 5 = 0, ZF=1
    jz zero_detected                # Переход, если ZF=1
    jmp not_zero
    
zero_detected:
    movq $1, %rbx                   # Индикатор что ZF сработал
    jmp continue_zf
    
not_zero:
    movq $0, %rbx
    
continue_zf:
    # === Флаг переноса (CF) ===
    movq $0xFFFFFFFFFFFFFFFF, %rax  # Максимальное беззнаковое число
    addq $1, %rax                   # Переполнение: 0xFFFF...FF + 1 = 0, CF=1
    jc carry_detected               # Переход если CF=1
    jmp no_carry
    
carry_detected:
    movq $1, %rcx                   # Индикатор переполнения
    jmp continue_cf
    
no_carry:
    movq $0, %rcx
    
continue_cf:
    # === Флаг знака (SF) ===
    movq $10, %rax
    subq $20, %rax                  # 10 - 20 = -10, SF=1 (отрицательное число)
    js negative_result              # Переход если SF=1
    jmp positive_result
    
negative_result:
    movq $1, %rdx                   # Индикатор отрицательного результата
    jmp continue_sf
    
positive_result:
    movq $0, %rdx
    
continue_sf:
    # === Флаг переполнения (OF) ===
    movq $0x7FFFFFFFFFFFFFFF, %rax  # Максимальное положительное число (signed)
    addq $1, %rax                   # Переполнение в знаковой арифметике, OF=1
    jo overflow_detected            # Переход если OF=1
    jmp no_overflow
    
overflow_detected:
    movq $1, %rsi                   # Индикатор переполнения
    jmp continue_of
    
no_overflow:
    movq $0, %rsi
    
continue_of:
    
    # Сравнение двух чисел
    movq $15, %rax
    movq $25, %rbx
    cmpq %rbx, %rax                 # Сравниваем 15 с 25
    
    # Различные условные переходы:
    jl less_than                    # SF != OF (меньше для знаковых)
    jb below                        # CF=1 (меньше для беззнаковых) 
    je equal                        # ZF=1 (равно)
    jg greater                      # ZF=0 AND SF=OF (больше для знаковых)
    ja above                        # CF=0 AND ZF=0 (больше для беззнаковых)
    jle less_equal                  # ZF=1 OR SF!=OF (меньше или равно)
    
less_than:
    movq $1, %r8                    # 15 < 25 (знаковое сравнение)
    jmp comparison_done
    
below:
    movq $2, %r8                    # 15 < 25 (беззнаковое сравнение)
    jmp comparison_done
    
equal:
    movq $3, %r8
    jmp comparison_done
    
greater:
    movq $4, %r8
    jmp comparison_done
    
above:
    movq $5, %r8
    jmp comparison_done
    
less_equal:
    movq $6, %r8
    jmp comparison_done
    
comparison_done:
    # === Использование флагов для реализации алгоритмов ===
    
    # Функция абсолютного значения
    movq $-42, %rax                 # Тестовое отрицательное число
    testq %rax, %rax                # Проверяем знак (не изменяет %rax!)
    jns already_positive            # Если SF=0, число положительное
    negq %rax                       # Делаем число положительным
    
already_positive:
    movq %rax, %r9                  # Результат: abs(-42) = 42
    
    # Функция maximum(a, b)
    movq $17, %rdi                  # a = 17
    movq $23, %rsi                  # b = 23
    cmpq %rsi, %rdi                 # Сравниваем a с b
    jge first_is_greater            # Если a >= b
    movq %rsi, %rax                 # max = b
    jmp max_done
    
first_is_greater:
    movq %rdi, %rax                 # max = a
    
max_done:
    movq %rax, %r10                 # Результат: max(17, 23) = 23
    
    # Завершение программы
    movq $60, %rax
    xorq %rdi, %rdi
    syscall
