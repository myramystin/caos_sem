#include <shared_mutex>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <thread>
#include <vector>

// === THREAD SAFETY ===
class ThreadSafeCounter {
private:
    mutable std::mutex mtx;
    int value = 0;

public:
    void increment() {
        std::lock_guard<std::mutex> lock(mtx);
        ++value;
    }
    
    int get() const {
        std::lock_guard<std::mutex> lock(mtx);
        return value;
    }
};

// === RWLOCK ===
class SharedData {
private:
    mutable std::shared_mutex rw_mutex;
    std::vector<int> data;

public:
    // Множественное чтение
    int read(size_t index) const {
        std::shared_lock<std::shared_mutex> lock(rw_mutex); // Разделяемая блокировка
        if (index < data.size()) {
            std::cout << "Читаем data[" << index << "] = " << data[index] 
                      << " (поток " << std::this_thread::get_id() << ")\n";
            return data[index];
        }
        return -1;
    }
    
    // Эксклюзивная запись
    void write(int value) {
        std::unique_lock<std::shared_mutex> lock(rw_mutex); // Эксклюзивная блокировка
        data.push_back(value);
        std::cout << "Записали " << value 
                  << " (поток " << std::this_thread::get_id() << ")\n";
    }
    
    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(rw_mutex);
        return data.size();
    }
};

// === СЕМАФОР ===
class Semaphore {
private:
    std::mutex mtx;
    std::condition_variable cv;
    int count;

public:
    Semaphore(int initial_count) : count(initial_count) {}
    
    void acquire() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return count > 0; });
        --count;
    }
    
    void release() {
        std::lock_guard<std::mutex> lock(mtx);
        ++count;
        cv.notify_one();
    }
};

void demo_synchronization() {
    std::cout << "\n=== ДЕМО: Thread Safety, RWLock, Семафор ===\n";
    
    // RWLock демо
    SharedData shared_data;
    std::vector<std::thread> threads;
    
    // Writer поток
    threads.emplace_back([&]() {
        for (int i = 0; i < 5; ++i) {
            shared_data.write(i * 10);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
    
    // Reader потоки (могут читать одновременно)
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < 3; ++j) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                if (shared_data.size() > 0) {
                    shared_data.read(0);
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Семафор демо - ограничиваем доступ к ресурсу
    std::cout << "\nСемафор (максимум 2 одновременных доступа):\n";
    Semaphore resource_semaphore(2);
    threads.clear();
    
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&, i]() {
            resource_semaphore.acquire();
            std::cout << "Поток " << i << " получил доступ к ресурсу\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            std::cout << "Поток " << i << " освобождает ресурс\n";
            resource_semaphore.release();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

int main() {
    demo_synchronization();
    return 0;
}
