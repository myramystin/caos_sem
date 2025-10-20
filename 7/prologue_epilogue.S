.section .text
.global main

# Функция с большим количеством параметров
# long sum_many(long a, long b, long c, long d, long e, long f, long g, long h)
# Первые 6 параметров в регистрах: %rdi, %rsi, %rdx, %rcx, %r8, %r9
# Остальные параметры в стеке
sum_many:
     # Шаг 1: Сохраняем старый frame pointer
    pushq %rbp                      # Стек: [...][old_rbp][ret_addr]
                                    # %rsp указывает на old_rbp
    
    # Шаг 2: Устанавливаем новый frame pointer
    movq %rsp, %rbp                 # %rbp = %rsp (указывает на сохраненный old_rbp)
                                   # Теперь %rbp - базовая точка для доступа к данным
    
    # Параметры:
    # %rdi = a, %rsi = b, %rdx = c, %rcx = d, %r8 = e, %r9 = f
    # 16(%rbp) = g, 24(%rbp) = h
    
    # Суммируем все параметры
    movq %rdi, %rax                 # a
    addq %rsi, %rax                 # + b
    addq %rdx, %rax                 # + c  
    addq %rcx, %rax                 # + d
    addq %r8, %rax                  # + e
    addq %r9, %rax                  # + f
    addq 16(%rbp), %rax             # + g (из стека)
    addq 24(%rbp), %rax             # + h (из стека)
    
    popq %rbp
    ret

main:
    # Подготавливаем параметры для вызова sum_many(1,2,3,4,5,6,7,8)
    
    # Сначала помещаем в стек параметры 7 и 8 (в обратном порядке!)
    pushq $8                        # h = 8
    pushq $7                        # g = 7
    
    # Затем загружаем регистры
    movq $1, %rdi                   # a = 1
    movq $2, %rsi                   # b = 2
    movq $3, %rdx                   # c = 3
    movq $4, %rcx                   # d = 4
    movq $5, %r8                    # e = 5
    movq $6, %r9                    # f = 6
    
    call sum_many
    
    # Очищаем стек от параметров (2 параметра по 8 байт = 16 байт)
    addq $16, %rsp
    
    # Результат должен быть 1+2+3+4+5+6+7+8 = 36
    movq %rax, %rdi
    movq $60, %rax
    syscall
