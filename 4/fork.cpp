#include <iostream>
#include <memory>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <stdexcept>
#include <cstring>
#include <cstdlib>
#include <sys/mman.h>
#include <fcntl.h>
#include <chrono>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

class ForkSemanticDemo {
private:
    int global_var = 100;
    static int static_var;
    
public:
    void demonstrate_fork_basics() {
        int local_var = 200;
        
        std::cout << "\n=== Базовая семантика fork ===" << std::endl;
        std::cout << "До fork: PID=" << getpid() 
                  << ", global_var=" << global_var 
                  << ", static_var=" << static_var
                  << ", local_var=" << local_var << std::endl;
        
        pid_t pid = fork();
        
        if (pid == -1) {
            throw std::runtime_error("fork failed");
        }
        
        if (pid == 0) {
            global_var = 300;
            static_var = 400;
            local_var = 500;
            
            std::cout << "ДОЧЕРНИЙ: PID=" << getpid() << ", PPID=" << getppid()
                      << ", global_var=" << global_var 
                      << ", static_var=" << static_var
                      << ", local_var=" << local_var << std::endl;
            
            std::this_thread::sleep_for(std::chrono::seconds(2));
            std::cout << "Дочерний завершается" << std::endl;
            _exit(0);
            
        } else {
            std::cout << "РОДИТЕЛЬ: PID=" << getpid() << ", child_PID=" << pid
                      << ", global_var=" << global_var 
                      << ", static_var=" << static_var
                      << ", local_var=" << local_var << std::endl;
            
            int status;
            wait(&status);
            
            std::cout << "РОДИТЕЛЬ: после изменений в дочернем - global_var=" << global_var 
                      << ", static_var=" << static_var << std::endl;
        }
    }
    
    void demonstrate_fd_inheritance() {
        std::cout << "\n=== Наследование файловых дескрипторов ===" << std::endl;
        
        int fd = open("fork_test.txt", O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd == -1) {
            throw std::runtime_error("open failed");
        }
        
        write(fd, "Initial data\n", 13);
        
        std::cout << "Файл открыт с дескриптором: " << fd << std::endl;
        
        pid_t pid = fork();
        
        if (pid == 0) {
            std::cout << "ДОЧЕРНИЙ: Дескриптор " << fd << " унаследован" << std::endl;
            
            write(fd, "Data from child\n", 16);
            
            off_t pos = lseek(fd, 0, SEEK_CUR);
            std::cout << "ДОЧЕРНИЙ: Позиция в файле: " << pos << std::endl;
            
            close(fd);
            std::cout << "ДОЧЕРНИЙ: Дескриптор закрыт в дочернем процессе" << std::endl;
            _exit(0);
            
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            off_t pos = lseek(fd, 0, SEEK_CUR);
            std::cout << "РОДИТЕЛЬ: Позиция в файле после записи дочернего: " << pos << std::endl;
            
            write(fd, "Data from parent\n", 17);
            close(fd);
            
            wait(nullptr);
            
            std::cout << "Содержимое файла после fork:" << std::endl;
            system("cat fork_test.txt");
        }
    }
    
    void demonstrate_copy_on_write() {
        std::cout << "\n=== Демонстрация Copy-on-Write ===" << std::endl;
        
        const size_t SIZE = 1024 * 1024; // 1MB
        
        std::unique_ptr<char[]> big_buffer = std::make_unique<char[]>(SIZE);
        
        memset(big_buffer.get(), 'A', SIZE);  // Используем C-style memset
        std::cout << "Выделено и заполнено " << SIZE << " байт" << std::endl;
        
        auto memory_before = get_memory_usage();
        std::cout << "RSS до fork: " << memory_before.rss_kb << " KB" << std::endl;
        
        pid_t pid = fork();
        
        if (pid == 0) {
            auto memory_after_fork = get_memory_usage();
            std::cout << "ДОЧЕРНИЙ: RSS сразу после fork: " << memory_after_fork.rss_kb << " KB" << std::endl;
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            std::cout << "ДОЧЕРНИЙ: Изменяем данные (запуск COW)" << std::endl;
            memset(big_buffer.get(), 'B', SIZE);  // C-style memset
            
            auto memory_after_cow = get_memory_usage();
            std::cout << "ДОЧЕРНИЙ: RSS после COW: " << memory_after_cow.rss_kb << " KB" << std::endl;
            
            std::this_thread::sleep_for(std::chrono::seconds(2));
            _exit(0);
            
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            auto memory_parent = get_memory_usage();
            std::cout << "РОДИТЕЛЬ: RSS после fork: " << memory_parent.rss_kb << " KB" << std::endl;
            
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            std::cout << "РОДИТЕЛЬ: Первые 10 байт: ";
            for (int i = 0; i < 10; ++i) {
                std::cout << big_buffer[i];
            }
            std::cout << std::endl;
            
            wait(nullptr);
        }
    }
    
    void demonstrate_mmap_inheritance() {
        std::cout << "\n=== Наследование mmap регионов ===" << std::endl;
        
        
        void* private_anon = mmap(nullptr, 4096, PROT_READ | PROT_WRITE,
                                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        
        void* shared_anon = mmap(nullptr, 4096, PROT_READ | PROT_WRITE,
                                 MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        
        if (private_anon == MAP_FAILED || shared_anon == MAP_FAILED) {
            throw std::runtime_error("mmap failed");
        }
        
        strcpy(static_cast<char*>(private_anon), "Private data");
        strcpy(static_cast<char*>(shared_anon), "Shared data");
        
        pid_t pid = fork();
        
        if (pid == 0) {
            std::cout << "ДОЧЕРНИЙ: Private mapping содержит: " << 
                         static_cast<char*>(private_anon) << std::endl;
            std::cout << "ДОЧЕРНИЙ: Shared mapping содержит: " << 
                         static_cast<char*>(shared_anon) << std::endl;
            
            strcpy(static_cast<char*>(private_anon), "Modified by child");
            strcpy(static_cast<char*>(shared_anon), "Modified by child");
            
            std::cout << "ДОЧЕРНИЙ: Данные изменены" << std::endl;
            _exit(0);
            
        } else {
            wait(nullptr);
            
            std::cout << "РОДИТЕЛЬ: Private mapping содержит: " << 
                         static_cast<char*>(private_anon) << std::endl;
            std::cout << "РОДИТЕЛЬ: Shared mapping содержит: " << 
                         static_cast<char*>(shared_anon) << std::endl;
            
            munmap(private_anon, 4096);
            munmap(shared_anon, 4096);
        }
    }
    
private:
    struct MemoryUsage {
        long rss_kb;
        long vsz_kb;
    };
    
    MemoryUsage get_memory_usage() {
        MemoryUsage usage = {};
        std::ifstream status("/proc/self/status");
        std::string line;
        
        while (std::getline(status, line)) {
            if (line.substr(0, 6) == "VmRSS:") {
                std::istringstream iss(line);
                std::string label;
                iss >> label >> usage.rss_kb;
            } else if (line.substr(0, 7) == "VmSize:") {
                std::istringstream iss(line);
                std::string label;
                iss >> label >> usage.vsz_kb;
            }
        }
        
        return usage;
    }
};

int ForkSemanticDemo::static_var = 150;

int main() {
    try {
        ForkSemanticDemo demo;
        
        demo.demonstrate_fork_basics();
        demo.demonstrate_fd_inheritance();
        demo.demonstrate_copy_on_write();
        demo.demonstrate_mmap_inheritance();
        
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
