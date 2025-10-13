.section .data
    array: .quad 15, 3, 28, 7, 42, 19, 5
    array_size: .quad 7

.section .bss
    .lcomm max_value, 8        # резервируем 8 байт для результата

.section .text
.global _start

_start:
    # Инициализация
    leaq array(%rip), %rsi      # RSI = указатель на массив
    movq array_size(%rip), %rcx # RCX = количество элементов
    movq (%rsi), %rax           # RAX = первый элемент как начальный максимум
    
.loop:
    cmpq $0, %rcx               # проверяем, есть ли еще элементы
    je .done                    # если нет, завершаем
    
    movq (%rsi), %rbx           # загружаем текущий элемент
    cmpq %rax, %rbx             # сравниваем с текущим максимумом
    jle .next                   # если меньше или равен, переходим к следующему
    
    movq %rbx, %rax             # обновляем максимум
    
.next:
    addq $8, %rsi               # переходим к следующему элементу (8 байт)
    decq %rcx                   # уменьшаем счетчик
    jmp .loop                   # повторяем цикл

.done:
    movq %rax, max_value(%rip)  # сохраняем результат
    
    # Выход с найденным максимумом как кодом возврата
    movq $60, %rax
    movq max_value(%rip), %rdi
    syscall
