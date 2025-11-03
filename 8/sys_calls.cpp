#include <unistd.h>
#include <iostream>

void my_write(const char* msg, size_t len) {
    long result;
    asm volatile (
        "syscall"
        : "=a" (result)
        : "a" (1),              // sys_write = 1
          "D" (1),              // fd = stdout = 1  
          "S" (msg),            // buffer = msg
          "d" (len)             // count = len
        : "rcx", "r11", "memory"
    );
}

void my_exit(int status) {
    asm volatile (
        "syscall"
        :
        : "a" (60),             // sys_exit = 60
          "D" (status)          // exit status
        :
    );
}

int main() {
    const char* msg = "Hello from syscall!\n";
    my_write(msg, 19);
    my_exit(0);

    std::cout << "here";
    return 0;
}
