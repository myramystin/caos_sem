.section .text
.global main

main:
    movq $15, %rax
    movq $10, %rbx
    
    cmpq %rbx, %rax         # Сравниваем rax и rbx (rax - rbx)
    je .equal               # Jump if Equal
    jg .greater             # Jump if Greater
    jl .less                # Jump if Less

.greater:
    # rax > rbx
    movq $1, %rcx
    jmp .end

.less:
    # rax < rbx
    movq $2, %rcx
    jmp .end

.equal:
    # rax == rbx
    movq $0, %rcx

.end:
    # Выход с результатом сравнения
    movq $60, %rax
    movq %rcx, %rdi
    syscall
