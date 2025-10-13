.section .text
.global _start

_start:
    movq $42, %rax        # Загружаем 42 в регистр rax
    movq %rax, %rbx       # Копируем значение из rax в rbx  
    addq %rbx, %rax       # rax = rax + rbx = 84
    
    # Выход из программы
    movq $60, %rax        # sys_exit
    movq $0, %rdi         # код возврата
    syscall
