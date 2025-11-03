extern "C" {
    // Прямые системные вызовы
    long syscall1(long syscall_number, long arg1) {
        long result;
        asm volatile (
            "syscall"
            : "=a" (result)
            : "a" (syscall_number), "D" (arg1)
            : "rcx", "r11"
        );
        return result;
    }
    
    long syscall3(long syscall_number, long arg1, long arg2, long arg3) {
        long result;
        asm volatile (
            "syscall"
            : "=a" (result)  
            : "a" (syscall_number), "D" (arg1), "S" (arg2), "d" (arg3)
            : "rcx", "r11"
        );
        return result;
    }
    
    void my_write(const char* str, int len) {
        syscall3(1, 1, (long)str, len); // sys_write
    }
    
    int my_strlen(const char* str) {
        int len = 0;
        while (str[len]) len++;
        return len;
    }
    
    void my_print(const char* str) {
        my_write(str, my_strlen(str));
    }
    
    void my_exit(int code) {
        syscall1(60, code); // sys_exit
    }
    
    void _start() {
        my_print("Hello from freestanding program!\n");
        my_exit(42);
    }
}
