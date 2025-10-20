.section .text

# extern "C" long fast_multiply(long a, long b);
.global fast_multiply
fast_multiply:
    pushq %rbp
    movq %rsp, %rbp
    
    movq %rdi, %rax         # a -> %rax
    imulq %rsi, %rax        # a * b
    
    popq %rbp
    ret

# extern "C" long factorial(int n);
.global factorial
factorial:
    pushq %rbp
    movq %rsp, %rbp
    
    movq %rdi, %rax         # n -> %rax
    movq $1, %rcx           # result = 1
    
factorial_loop:
    cmpq $1, %rax
    jle factorial_done
    
    imulq %rax, %rcx        # result *= n
    decq %rax               # n--
    jmp factorial_loop
    
factorial_done:
    movq %rcx, %rax         # return result
    
    popq %rbp
    ret

# extern "C" void copy_memory(void* dest, void* src, size_t count);
.global copy_memory
copy_memory:
    pushq %rbp
    movq %rsp, %rbp
    
    # %rdi = dest, %rsi = src, %rdx = count
    movq %rdx, %rcx         # count -> %rcx
    
copy_loop:
    testq %rcx, %rcx
    jz copy_done
    
    movb (%rsi), %al        # Загружаем байт из source
    movb %al, (%rdi)        # Сохраняем в destination
    
    incq %rsi               # src++
    incq %rdi               # dest++
    decq %rcx               # count--
    
    jmp copy_loop
    
copy_done:
    popq %rbp
    ret

# extern "C" int string_length(const char* str);
.global string_length
string_length:
    pushq %rbp
    movq %rsp, %rbp
    
    movq %rdi, %rsi         # str -> %rsi
    xorq %rax, %rax         # length = 0
    
strlen_loop:
    movb (%rsi), %cl        # Загружаем текущий символ
    testb %cl, %cl          # Проверяем на '\0'
    jz strlen_done
    
    incq %rax               # length++
    incq %rsi               # str++
    jmp strlen_loop
    
strlen_done:
    popq %rbp
    ret
