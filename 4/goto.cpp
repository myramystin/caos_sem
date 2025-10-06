#include <iostream>
#include <memory>
#include <fstream>
#include <fcntl.h>
#include <stdexcept>
#include <sys/mman.h>
#include <unistd.h>
#include <cerrno>

class ResourceManagerBad {
public:
    int bad_resource_management() {
        int fd1 = open("file1.txt", O_RDONLY);
        if (fd1 == -1) {
            return -1;
        }
        
        int fd2 = open("file2.txt", O_RDONLY);
        if (fd2 == -1) {
            close(fd1);
            return -1;
        }
        
        std::unique_ptr<char[]> buffer(new char[1024]);
        if (!buffer) {
            close(fd1);
            close(fd2);
            return -1;
        }
        
        void* mapped_mem = mmap(nullptr, 4096, PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (mapped_mem == MAP_FAILED) {
            close(fd1);
            close(fd2);
            return -1;
        }
        
        int result = some_operation(fd1, fd2, buffer.get(), mapped_mem);
        
        if (result < 0) {
            munmap(mapped_mem, 4096);
            close(fd1);
            close(fd2);
            return -1;
        }
        
        munmap(mapped_mem, 4096);
        close(fd1);
        close(fd2);
        return 0;
    }
    
private:
    int some_operation(int fd1, int fd2, char* buffer, void* mem) {
        return 0;
    }
};

class ResourceManagerGood {
public:
    int good_resource_management() {
        int fd1 = -1, fd2 = -1;
        std::unique_ptr<char[]> buffer;
        void* mapped_mem = MAP_FAILED;
        int result = -1;
        
        fd1 = open("file1.txt", O_RDONLY);
        if (fd1 == -1) {
            goto cleanup;
        }
        
        fd2 = open("file2.txt", O_RDONLY);
        if (fd2 == -1) {
            goto cleanup;
        }
        
        buffer = std::make_unique<char[]>(1024);
        if (!buffer) {
            goto cleanup;
        }
        
        mapped_mem = mmap(nullptr, 4096, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (mapped_mem == MAP_FAILED) {
            goto cleanup;
        }
        
        result = some_operation(fd1, fd2, buffer.get(), mapped_mem);
        
    cleanup:
        if (mapped_mem != MAP_FAILED) {
            munmap(mapped_mem, 4096);
        }
        if (fd2 != -1) close(fd2);
        if (fd1 != -1) close(fd1);
        
        return result;
    }
    
private:
    int some_operation(int fd1, int fd2, char* buffer, void* mem) {
        return 0;
    }
};

class ResourceManagerRAII {
private:
    class FileDescriptor {
        int fd_;
    public:
        explicit FileDescriptor(const char* filename) 
            : fd_(open(filename, O_RDONLY)) {
            if (fd_ == -1) {
                throw std::runtime_error("Failed to open file");
            }
        }
        
        ~FileDescriptor() {
            if (fd_ != -1) {
                close(fd_);
            }
        }
        
        int get() const { return fd_; }
        
        FileDescriptor(const FileDescriptor&) = delete;
        FileDescriptor& operator=(const FileDescriptor&) = delete;
        
        FileDescriptor(FileDescriptor&& other) noexcept : fd_(other.fd_) {
            other.fd_ = -1;
        }
    };
    
    class MappedMemory {
        void* ptr_;
        size_t size_;
    public:
        explicit MappedMemory(size_t size) : size_(size) {
            ptr_ = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (ptr_ == MAP_FAILED) {
                throw std::runtime_error("mmap failed");
            }
        }
        
        ~MappedMemory() {
            if (ptr_ != MAP_FAILED) {
                munmap(ptr_, size_);
            }
        }
        
        void* get() const { return ptr_; }
        
        MappedMemory(const MappedMemory&) = delete;
        MappedMemory& operator=(const MappedMemory&) = delete;
        
        MappedMemory(MappedMemory&& other) noexcept 
            : ptr_(other.ptr_), size_(other.size_) {
            other.ptr_ = MAP_FAILED;
        }
    };
    
public:
    int raii_resource_management() {
        try {
            system("echo 'test1' > file1.txt");
            system("echo 'test2' > file2.txt");
            
            FileDescriptor fd1("file1.txt");
            FileDescriptor fd2("file2.txt");
            auto buffer = std::make_unique<char[]>(1024);
            MappedMemory mapped_mem(4096);
            
            return some_operation(fd1.get(), fd2.get(), buffer.get(), mapped_mem.get());
            
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return -1;
        }
    }
    
private:
    int some_operation(int fd1, int fd2, char* buffer, void* mem) {
        return 0;
    }
};

class KernelStyleGoto {
private:
    struct Resource {
        int type;
        void* data;
    };
    
public:
    int kernel_style_function() {
        Resource* res1 = nullptr;
        Resource* res2 = nullptr;
        Resource* res3 = nullptr;
        int ret = -1;
        
        res1 = allocate_resource(1);
        if (!res1) {
            ret = -ENOMEM;
            goto out;
        }
        
        res2 = allocate_resource(2);
        if (!res2) {
            ret = -ENOMEM;
            goto out_free_res1;
        }
        
        res3 = allocate_resource(3);
        if (!res3) {
            ret = -ENOMEM;
            goto out_free_res2;
        }
        
        ret = do_work(res1, res2, res3);
        if (ret < 0)
            goto out_free_res3;
        
        ret = 0;
        
    out_free_res3:
        free_resource(res3);
    out_free_res2:
        free_resource(res2);
    out_free_res1:
        free_resource(res1);
    out:
        return ret;
    }
    
private:
    Resource* allocate_resource(int type) {
        if (type == 2) {
            std::cout << "Simulating allocation failure for resource type " << type << std::endl;
            return nullptr;
        }
        
        Resource* res = new Resource{type, reinterpret_cast<void*>(0x1000 + type)};
        std::cout << "Allocated resource type " << type << " at " << res << std::endl;
        return res;
    }
    
    void free_resource(Resource* res) {
        if (res) {
            std::cout << "Freed resource type " << res->type << " at " << res << std::endl;
            delete res;
        }
    }
    
    int do_work(Resource* r1, Resource* r2, Resource* r3) {
        std::cout << "Working with resources..." << std::endl;
        return 0;
    }
};

int main() {
    try {
        std::cout << "=== Демонстрация управления ресурсами ===" << std::endl;
        
        system("echo 'test content 1' > file1.txt");
        system("echo 'test content 2' > file2.txt");
        
        std::cout << "\n1. Плохой подход без goto:" << std::endl;
        ResourceManagerBad bad_manager;
        int result1 = bad_manager.bad_resource_management();
        std::cout << "Result: " << result1 << std::endl;
        
        std::cout << "\n2. Хороший подход с goto:" << std::endl;
        ResourceManagerGood good_manager;
        int result2 = good_manager.good_resource_management();
        std::cout << "Result: " << result2 << std::endl;
        
        std::cout << "\n3. RAII подход (современный C++):" << std::endl;
        ResourceManagerRAII raii_manager;
        int result3 = raii_manager.raii_resource_management();
        std::cout << "Result: " << result3 << std::endl;
        
        std::cout << "\n4. Kernel-style goto:" << std::endl;
        KernelStyleGoto kernel_manager;
        int result4 = kernel_manager.kernel_style_function();
        std::cout << "Result: " << result4 << std::endl;
        
        system("rm -f file1.txt file2.txt");
        
        std::cout << "\n=== Все демонстрации завершены ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
