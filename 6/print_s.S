.section .data
    msg: .ascii "Hello, Assembly World!\n"
    msg_len = . - msg

.section .text
.global _start

_start:
    # Выводим строку
    movq $1, %rax           # sys_write
    movq $1, %rdi           # stdout
    movq $msg, %rsi         # адрес строки  
    movq $msg_len, %rdx     # длина строки
    syscall
    
    # Выходим
    movq $60, %rax          # sys_exit
    movq $0, %rdi           # код возврата 0
    syscall
