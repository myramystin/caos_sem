#include <sys/mman.h>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <memory>
#include <stdexcept>

class MemoryProtectionDemo {
private:
    void* ptr;
    size_t size;

public:
    MemoryProtectionDemo(size_t page_size = 4096) : size(page_size) {
        
        ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, 
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        
        if (ptr == MAP_FAILED) {
            throw std::runtime_error("mmap failed");
        }
    }
    
    ~MemoryProtectionDemo() {
        if (ptr != MAP_FAILED) {
            munmap(ptr, size);
        }
    }
    
    void demonstrate_mprotect() {
        std::cout << "\n=== Демонстрация mprotect ===" << std::endl;
        
        
        *static_cast<int*>(ptr) = 42;
        std::cout << "Значение после записи: " << *static_cast<int*>(ptr) << std::endl;
        
        
        if (mprotect(ptr, size, PROT_READ) == -1) {
            perror("mprotect");
            return;
        }
        
        std::cout << "Память переведена в режим только чтения" << std::endl;
        std::cout << "Значение при чтении: " << *static_cast<int*>(ptr) << std::endl;
        
        std::cout << "Попытка записи в read-only память вызовет SIGSEGV (закомментирована)" << std::endl;
        
        
        
        
        if (mprotect(ptr, size, PROT_READ | PROT_WRITE) == -1) {
            perror("mprotect restore");
            return;
        }
        
        *static_cast<int*>(ptr) = 100;
        std::cout << "После восстановления прав записи: " << *static_cast<int*>(ptr) << std::endl;
    }
    
    void demonstrate_execute_protection() {
        std::cout << "\n=== Демонстрация защиты от выполнения ===" << std::endl;
        
        
        if (mprotect(ptr, size, PROT_READ | PROT_WRITE | PROT_EXEC) == -1) {
            perror("mprotect execute");
            return;
        }
        
        std::cout << "Память помечена как исполняемая" << std::endl;
        
        
        if (mprotect(ptr, size, PROT_READ | PROT_WRITE) == -1) {
            perror("mprotect no execute");
            return;
        }
        
        std::cout << "Права выполнения убраны (NX bit установлен)" << std::endl;
    }
};

class MremapDemo {
public:
    void demonstrate_mremap() {
        std::cout << "\n=== Демонстрация mremap ===" << std::endl;
        
        const size_t initial_size = 4096;
        const size_t new_size = 8192;
        
        
        void* ptr = mmap(nullptr, initial_size, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        
        if (ptr == MAP_FAILED) {
            throw std::runtime_error("mmap failed");
        }
        
        std::strcpy(static_cast<char*>(ptr), "Тестовые данные для mremap");
        std::cout << "Данные до mremap: " << static_cast<char*>(ptr) << std::endl;
        
        
        ptr = mremap(ptr, initial_size, new_size, MREMAP_MAYMOVE);
        if (ptr == MAP_FAILED) {
            perror("mremap");
            return;
        }
        
        std::cout << "Данные сохранились после mremap: " << static_cast<char*>(ptr) << std::endl;
        
        
        std::strcpy(static_cast<char*>(ptr) + 4096, " + дополнительные данные");
        std::cout << "Полные данные: " << static_cast<char*>(ptr) << std::endl;
        
        munmap(ptr, new_size);
    }
    
    void demonstrate_mremap_fixed() {
        std::cout << "\n=== Демонстрация mremap с уменьшением размера ===" << std::endl;
        
        const size_t initial_size = 8192;  
        const size_t new_size = 4096;      
        
        void* ptr = mmap(nullptr, initial_size, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        
        if (ptr == MAP_FAILED) {
            throw std::runtime_error("mmap failed");
        }
        
        
        std::strcpy(static_cast<char*>(ptr), "Первая страница");
        std::strcpy(static_cast<char*>(ptr) + 4096, "Вторая страница");
        
        std::cout << "До mremap - первая страница: " << static_cast<char*>(ptr) << std::endl;
        std::cout << "До mremap - вторая страница: " << 
                     static_cast<char*>(ptr) + 4096 << std::endl;
        
        
        ptr = mremap(ptr, initial_size, new_size, 0);  
        if (ptr == MAP_FAILED) {
            perror("mremap shrink");
            return;
        }
        
        std::cout << "После mremap - первая страница: " << static_cast<char*>(ptr) << std::endl;
        std::cout << "Вторая страница теперь недоступна" << std::endl;
        
        munmap(ptr, new_size);
    }
};

class MadviseDemo {
public:
    void demonstrate_madvise() {
        std::cout << "\n=== Демонстрация MADV_DONTNEED ===" << std::endl;
        
        const size_t size = 1024 * 1024; 
        
        void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        
        if (ptr == MAP_FAILED) {
            throw std::runtime_error("mmap failed");
        }
        
        
        memset(ptr, 42, size);  
        std::cout << "Первый байт после заполнения: " << 
                     static_cast<int>(*static_cast<char*>(ptr)) << std::endl;
        
        
        
        if (madvise(ptr, size, MADV_DONTNEED) == -1) {
            perror("madvise");
        } else {
            std::cout << "MADV_DONTNEED применен" << std::endl;
        }
        
        
        std::cout << "Первый байт после MADV_DONTNEED: " << 
                     static_cast<int>(*static_cast<char*>(ptr)) << std::endl; 
        
        munmap(ptr, size);
    }
    
    void demonstrate_prefetch() {
        std::cout << "\n=== Демонстрация MADV_SEQUENTIAL и MADV_WILLNEED ===" << std::endl;
        
        const size_t size = 1024 * 1024; 
        
        void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        
        if (ptr == MAP_FAILED) {
            throw std::runtime_error("mmap failed");
        }
        
        
        if (madvise(ptr, size, MADV_SEQUENTIAL) == -1) {
            perror("madvise MADV_SEQUENTIAL");
        } else {
            std::cout << "MADV_SEQUENTIAL применен" << std::endl;
        }
        
        
        if (madvise(ptr, size, MADV_WILLNEED) == -1) {
            perror("madvise MADV_WILLNEED");
        } else {
            std::cout << "MADV_WILLNEED применен" << std::endl;
        }
        
        std::cout << "Выполняем последовательный доступ..." << std::endl;
        
        
        char sum = 0;
        for (size_t i = 0; i < size; ++i) {
            static_cast<char*>(ptr)[i] = i % 256;
            sum += static_cast<char*>(ptr)[i];
        }
        
        std::cout << "Контрольная сумма: " << static_cast<int>(sum) << std::endl;
        
        munmap(ptr, size);
    }
    
    void demonstrate_random_access() {
        std::cout << "\n=== Демонстрация MADV_RANDOM ===" << std::endl;
        
        const size_t size = 1024 * 1024; 
        
        void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        
        if (ptr == MAP_FAILED) {
            throw std::runtime_error("mmap failed");
        }
        
        
        if (madvise(ptr, size, MADV_RANDOM) == -1) {
            perror("madvise MADV_RANDOM");
        } else {
            std::cout << "MADV_RANDOM применен" << std::endl;
        }
        
        
        char* mem = static_cast<char*>(ptr);
        for (int i = 0; i < 1000; ++i) {
            size_t random_offset = (i * 1024 + i * 13) % size;
            mem[random_offset] = i % 256;
        }
        
        std::cout << "Выполнен случайный доступ к памяти" << std::endl;
        
        munmap(ptr, size);
    }
};

int main() {
    try {
        std::cout << "=== Демонстрация системных вызовов управления памятью ===" << std::endl;
        
        
        MemoryProtectionDemo protection_demo;
        protection_demo.demonstrate_mprotect();
        protection_demo.demonstrate_execute_protection();
        
        
        MremapDemo remap_demo;
        remap_demo.demonstrate_mremap();
        remap_demo.demonstrate_mremap_fixed();
        
        
        MadviseDemo advise_demo;
        advise_demo.demonstrate_madvise();
        advise_demo.demonstrate_prefetch();
        advise_demo.demonstrate_random_access();
        
        std::cout << "\n=== Все демонстрации управления памятью завершены ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
