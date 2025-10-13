.section .text
.global _start

_start:
    movq $-1, %rax           # rax = 0xFFFFFFFFFFFFFFFF (все биты установлены)
    movl $42, %eax           # Теперь rax = 0x000000000000002A (старшие биты обнулились!)
    
    movq $-1, %rax           # Снова устанавливаем все биты
    movw $42, %ax            # Теперь rax = 0xFFFFFFFFFFFF002A (старшие биты сохранились)
    
    movq $-1, %rax           # Снова все биты
    movb $42, %al            # Теперь rax = 0xFFFFFFFFFFFFFF2A (изменился только младший байт)
    