#include <iostream>
#include <thread>
#include <vector>
#include <sstream>
#include <random>
#include <mutex>

// Глобальная thread-local переменная
thread_local int tl_counter = 0;
thread_local std::string tl_thread_name;

// Обычная глобальная переменная для сравнения
int global_counter = 0;
std::mutex global_mutex;

void worker_function(int thread_id) {
    tl_thread_name = "Thread-" + std::to_string(thread_id);
    
    std::cout << "Starting " << tl_thread_name << std::endl;
    
    // Работа с thread-local счетчиком
    for (int i = 0; i < 5; ++i) {
        tl_counter++;
        
        // Также увеличиваем глобальный счетчик (требует синхронизации)
        {
            std::lock_guard<std::mutex> lock(global_mutex);
            global_counter++;
        }
        
        std::cout << tl_thread_name << ": local=" << tl_counter 
                  << ", global=" << global_counter << std::endl;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << tl_thread_name << " final local counter: " << tl_counter << std::endl;
}

// Класс с thread-local членами
class ThreadLocalLogger {
private:
    static thread_local std::ostringstream buffer_;
    static thread_local int message_count_;

public:
    static void log(const std::string& message) {
        message_count_++;
        buffer_ << "[" << std::this_thread::get_id() << "][" 
                << message_count_ << "] " << message << "\n";
    }
    
    static std::string get_log() {
        return buffer_.str();
    }
    
    static void clear() {
        buffer_.str("");
        buffer_.clear();
        message_count_ = 0;
    }
    
    static int get_message_count() {
        return message_count_;
    }
};

// Определение статических thread-local членов
thread_local std::ostringstream ThreadLocalLogger::buffer_;
thread_local int ThreadLocalLogger::message_count_ = 0;

void demonstrate_thread_local_logger() {
    std::cout << "\n=== Thread Local Logger Demo ===" << std::endl;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([i]() {
            ThreadLocalLogger::log("Starting thread " + std::to_string(i));
            
            for (int j = 0; j < 3; ++j) {
                ThreadLocalLogger::log("Message " + std::to_string(j) + 
                                     " from thread " + std::to_string(i));
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            
            ThreadLocalLogger::log("Finishing thread " + std::to_string(i));
            
            std::cout << "=== Thread " << i << " Log ===" << std::endl;
            std::cout << ThreadLocalLogger::get_log();
            std::cout << "Total messages: " << ThreadLocalLogger::get_message_count() << std::endl;
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

// Thread-local random number generator
thread_local std::random_device rd;
thread_local std::mt19937 gen(rd());
thread_local std::uniform_int_distribution<> dis(1, 100);

int get_random() {
    return dis(gen);
}

void demonstrate_thread_local_rng() {
    std::cout << "\n=== Thread Local RNG Demo ===" << std::endl;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([i]() {
            std::cout << "Thread " << i << " random numbers: ";
            for (int j = 0; j < 5; ++j) {
                std::cout << get_random() << " ";
            }
            std::cout << std::endl;
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

class ThreadLocalResource {
private:
    int id_;
    
public:
    ThreadLocalResource(int id) : id_(id) {
        std::cout << "ThreadLocalResource " << id_ << " created in thread " 
                  << std::this_thread::get_id() << std::endl;
    }
    
    ~ThreadLocalResource() {
        std::cout << "ThreadLocalResource " << id_ << " destroyed in thread " 
                  << std::this_thread::get_id() << std::endl;
    }
    
    int get_id() const { return id_; }
};

thread_local ThreadLocalResource* tl_resource = nullptr;

ThreadLocalResource& get_thread_resource() {
    if (!tl_resource) {
        static thread_local int counter = 0;
        tl_resource = new ThreadLocalResource(++counter);
    }
    return *tl_resource;
}

void cleanup_thread_resource() {
    delete tl_resource;
    tl_resource = nullptr;
}

void demonstrate_thread_local_lifetime() {
    std::cout << "\n=== Thread Local Lifetime Demo ===" << std::endl;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([i]() {
            std::cout << "Thread " << i << " accessing resource..." << std::endl;
            
            // Первое обращение создает ресурс
            auto& resource1 = get_thread_resource();
            std::cout << "Thread " << i << " got resource " << resource1.get_id() << std::endl;
            
            // Второе обращение возвращает тот же ресурс
            auto& resource2 = get_thread_resource();
            std::cout << "Thread " << i << " got same resource " << resource2.get_id() 
                      << " (same object: " << (&resource1 == &resource2) << ")" << std::endl;
            
            // Явная очистка (обычно не нужна - ресурс уничтожится при завершении потока)
            cleanup_thread_resource();
            std::cout << "Thread " << i << " cleaned up" << std::endl;
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

int main() {
    std::cout << "=== Basic Thread Local Demo ===" << std::endl;
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back(worker_function, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "Final global counter: " << global_counter << std::endl;
    
    demonstrate_thread_local_logger();
    demonstrate_thread_local_rng();
    demonstrate_thread_local_lifetime();
    
    return 0;
}
