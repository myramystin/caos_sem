#include <iostream>
#include <chrono>
#include <cstring>

extern "C" {
    long fast_multiply(long a, long b);
    long factorial(int n);
    void copy_memory(void* dest, void* src, size_t count);
    int string_length(const char* str);
}


int main() {
    std::cout << "=== Assembly Functions Demo ===\n\n";
    
    // Тест умножения
    long a = 12345, b = 67890;
    long result_asm = fast_multiply(a, b);
    long result_cpp = a * b;
    
    std::cout << "Multiplication Test:\n";
    std::cout << "Assembly: " << a << " * " << b << " = " << result_asm << '\n';
    std::cout << "C++:      " << a << " * " << b << " = " << result_cpp << '\n';
    std::cout << "Match: " << (result_asm == result_cpp ? "YES" : "NO") << "\n\n";
    
    // Тест факториала
    int n = 10;
    long fact_result = factorial(n);
    std::cout << "Factorial Test:\n";
    std::cout << "factorial(" << n << ") = " << fact_result << "\n\n";
    
    // Тест копирования памяти
    const char source[] = "Hello, Assembly World!";
    char dest1[100], dest2[100];
    
    copy_memory(dest1, (void*)source, strlen(source) + 1);
    strcpy(dest2, source);
    
    std::cout << "Memory Copy Test:\n";
    std::cout << "Assembly copy: \"" << dest1 << "\"\n";
    std::cout << "Standard copy: \"" << dest2 << "\"\n";
    std::cout << "Match: " << (strcmp(dest1, dest2) == 0 ? "YES" : "NO") << "\n\n";
    
    // Тест длины строки
    const char test_string[] = "Testing string length function";
    int asm_len = string_length(test_string);
    int std_len = strlen(test_string);
    
    std::cout << "String Length Test:\n";
    std::cout << "String: \"" << test_string << "\"\n";
    std::cout << "Assembly length: " << asm_len << '\n';
    std::cout << "Standard length: " << std_len << '\n';
    std::cout << "Match: " << (asm_len == std_len ? "YES" : "NO") << "\n\n";
    
    return 0;
}
