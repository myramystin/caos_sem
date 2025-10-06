#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <vector>
#include <iomanip>
#include <cstdint>


void demonstrate_virtual_addresses() {
    std::cout << "PID процесса: " << getpid() << std::endl;
    
    
    // 1. Глобальные переменные (в сегменте данных)
    static int global_var = 42;
    
    // 2. Локальные переменные (на стеке)
    int local_var = 10;
    
    // 3. Динамически выделенная память (в куче)
    int* heap_var = new int(20);
    
    // 4. Константы (в сегменте кода/констант)
    const char* const_string = "Hello, World!";
    
    // 5. Адрес функции (в сегменте кода)
    void* func_ptr = (void*)demonstrate_virtual_addresses;
    
    std::cout << std::hex << std::showbase;
    std::cout << "Глобальная переменная:  " << &global_var << std::endl;
    std::cout << "Локальная переменная:   " << &local_var << std::endl;
    std::cout << "Переменная в куче:      " << heap_var << std::endl;
    std::cout << "Строковая константа:    " << (void*)const_string << std::endl;
    std::cout << "Адрес функции:          " << func_ptr << std::endl;
    
    std::cout << std::dec << std::noshowbase;
    
    uintptr_t stack_addr = (uintptr_t)&local_var;
    uintptr_t heap_addr = (uintptr_t)heap_var;
    uintptr_t global_addr = (uintptr_t)&global_var;
    uintptr_t code_addr = (uintptr_t)func_ptr;
    
    std::cout << "Стек обычно находится в верхней части адресного пространства" << std::endl;
    std::cout << "Куча растет вверх от нижних адресов" << std::endl;
    std::cout << "Код программы обычно загружается в начало" << std::endl;
    
    if (stack_addr > heap_addr) {
        std::cout << "✓ Стек действительно выше кучи в адресном пространстве" << std::endl;
    }
    
    delete heap_var;
}


void demonstrate_page_size() {
    std::cout << "\n=== Информация о страницах памяти ===" << std::endl;
    
    long page_size = sysconf(_SC_PAGESIZE);
    std::cout << "Размер страницы в системе: " << page_size << " байт (" 
              << page_size / 1024 << " KB)" << std::endl;
    
    char* memory1 = new char[1];
    char* memory2 = new char[1];
    char* memory3 = new char[page_size];
    
    std::cout << "Небольшие выделения:" << std::endl;
    std::cout << "  memory1: " << (void*)memory1 << std::endl;
    std::cout << "  memory2: " << (void*)memory2 << std::endl;
    std::cout << "  Разница: " << (memory2 - memory1) << " байт" << std::endl;
    
    std::cout << "Большое выделение:" << std::endl;
    std::cout << "  memory3: " << (void*)memory3 << std::endl;
    
    uintptr_t addr = (uintptr_t)memory3;
    if (addr % page_size == 0) {
        std::cout << "  ✓ Выровнено по границе страницы" << std::endl;
    } else {
        std::cout << "  Смещение от границы страницы: " << (addr % page_size) << " байт" << std::endl;
    }
    
    delete[] memory1;
    delete[] memory2;
    delete[] memory3;
}

int main() {
    demonstrate_virtual_addresses();
    demonstrate_page_size();
    
    std::cout << "cat /proc/" << getpid() << "/maps" << std::endl;

    std::cin.get();

    
    return 0;
}
