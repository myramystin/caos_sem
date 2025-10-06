#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

void demonstrate_mmap_parameters() {
    
    long page_size = sysconf(_SC_PAGESIZE);
    std::cout << "Размер страницы: " << page_size << " байт" << std::endl;
    
    std::cout << "\n--- Различные права доступа ---" << std::endl;
    
    void* read_only = mmap(NULL, page_size, PROT_READ,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    std::cout << "Read-only память: " << read_only << std::endl;
    
    void* read_write = mmap(NULL, page_size, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    std::cout << "Read-write память: " << read_write << std::endl;
    
    // *((char*)read_only) = 'A'; // <- Это вызовет ошибку!
    
    *((char*)read_write) = 'B';
    std::cout << "Записали в read-write память: " << *((char*)read_write) << std::endl;
    
    size_t small_size = 100; // Меньше размера страницы
    void* small_mapping = mmap(NULL, small_size, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    std::cout << "Запросили " << small_size << " байт, получили как минимум " 
              << page_size << " байт" << std::endl;
    std::cout << "Адрес: " << small_mapping << std::endl;
    
    char* small_ptr = (char*)small_mapping;
    small_ptr[0] = 'S';
    small_ptr[small_size - 1] = 'E';
    small_ptr[page_size - 1] = 'P';
    
    munmap(read_only, page_size);
    munmap(read_write, page_size);
    munmap(small_mapping, small_size);
}

void demonstrate_file_mapping_detailed() {
    
    const char* filename = "test_mmap_file.txt";
    const char* test_data = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    size_t data_size = strlen(test_data);
    
    int fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return;
    }
    
    if (write(fd, test_data, data_size) != (ssize_t)data_size) {
        perror("write");
        close(fd);
        return;
    }
    
    std::cout << "Создан файл размером " << data_size << " байт" << std::endl;
    
    std::cout << "\n1. Отображение всего файла:" << std::endl;
    void* full_mapping = mmap(NULL, data_size, PROT_READ,
                             MAP_PRIVATE, fd, 0);
    if (full_mapping == MAP_FAILED) {
        perror("mmap full file");
    } else {
        char* full_data = (char*)full_mapping;
        std::cout << "Содержимое: ";
        for (size_t i = 0; i < data_size; ++i) {
            std::cout << full_data[i];
        }
        std::cout << std::endl;
        munmap(full_mapping, data_size);
    }
    
    std::cout << "\n2. Отображение с offset 10:" << std::endl;
    long page_size = sysconf(_SC_PAGESIZE);
    off_t offset = 0; // offset должен быть кратен размеру страницы!
    size_t mapping_size = page_size; // Отображаем целую страницу
    
    void* partial_mapping = mmap(NULL, mapping_size, PROT_READ,
                                MAP_PRIVATE, fd, offset);
    if (partial_mapping == MAP_FAILED) {
        perror("mmap with offset");
    } else {
        char* partial_data = (char*)partial_mapping;
        std::cout << "Содержимое с offset " << offset << ": ";
        size_t actual_offset = 10;
        for (size_t i = actual_offset; i < data_size && i < mapping_size; ++i) {
            std::cout << partial_data[i];
        }
        std::cout << std::endl;
        munmap(partial_mapping, mapping_size);
    }
    
    // MAP_SHARED - изменения записываются в файл
    void* shared_mapping = mmap(NULL, data_size, PROT_READ | PROT_WRITE,
                               MAP_SHARED, fd, 0);
    if (shared_mapping != MAP_FAILED) {
        char* shared_data = (char*)shared_mapping;
        shared_data[0] = 'X';
        
        msync(shared_mapping, data_size, MS_SYNC);
        std::cout << "MAP_SHARED: изменили первый символ на 'X'" << std::endl;
        munmap(shared_mapping, data_size);
    }
    
    lseek(fd, 0, SEEK_SET);
    char check_buffer[10];
    read(fd, check_buffer, 1);
    std::cout << "Первый символ в файле теперь: '" << check_buffer[0] << "'" << std::endl;
    
    void* private_mapping = mmap(NULL, data_size, PROT_READ | PROT_WRITE,
                                MAP_PRIVATE, fd, 0);
    if (private_mapping != MAP_FAILED) {
        char* private_data = (char*)private_mapping;
        private_data[1] = 'Y'; 
        std::cout << "MAP_PRIVATE: изменили второй символ на 'Y' (только в памяти)" << std::endl;
        munmap(private_mapping, data_size);
    }
    
    lseek(fd, 1, SEEK_SET);
    read(fd, check_buffer, 1);
    std::cout << "Второй символ в файле остался: '" << check_buffer[0] << "'" << std::endl;
    
    close(fd);
    unlink(filename);
}

void demonstrate_error_handling() {
    std::cout << "\n=== Обработка ошибок mmap ===" << std::endl;
    
    void* bad_mapping1 = mmap(NULL, 4096, PROT_READ, MAP_PRIVATE, -1, 0);
    if (bad_mapping1 == MAP_FAILED) {
        std::cout << "Ошибка отображения с fd=-1 без MAP_ANONYMOUS: " 
                  << strerror(errno) << std::endl;
    }
    
    const char* temp_file = "readonly_test.txt";
    int readonly_fd = open(temp_file, O_CREAT | O_RDONLY, 0644);
    if (readonly_fd != -1) {
        void* bad_mapping2 = mmap(NULL, 4096, PROT_WRITE, MAP_SHARED, readonly_fd, 0);
        if (bad_mapping2 == MAP_FAILED) {
            std::cout << "Ошибка PROT_WRITE для read-only файла: " 
                      << strerror(errno) << std::endl;
        }
        close(readonly_fd);
        unlink(temp_file);
    }
    
    int temp_fd = open("temp_file.txt", O_CREAT | O_RDWR, 0644);
    if (temp_fd != -1) {
        ftruncate(temp_fd, 8192);
        
        void* bad_mapping3 = mmap(NULL, 4096, PROT_READ, MAP_PRIVATE, temp_fd, 100);
        if (bad_mapping3 == MAP_FAILED) {
            std::cout << "Ошибка с неправильным offset (100): " 
                      << strerror(errno) << std::endl;
        }
        
        close(temp_fd);
        unlink("temp_file.txt");
    }
}

int main() {
    std::cout << "PID процесса: " << getpid() << std::endl;
    
    demonstrate_mmap_parameters();
    demonstrate_file_mapping_detailed();
    demonstrate_error_handling();
    
    std::cout << "\nПосмотрите /proc/" << getpid() << "/maps для анализа отображений" << std::endl;
    std::cin.get();
    
    return 0;
}
