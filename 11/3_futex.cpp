#include <iostream>
#include <thread>
#include <atomic>
#include <vector>

class SimpleFutex {
private:
    std::atomic<int> futex_word{0};  // 0 = свободно, 1 = заблокировано
    
public:
    void lock() {
        int expected = 0;
        // Быстрый путь: пытаемся захватить атомарно
        if (futex_word.compare_exchange_strong(expected, 1, std::memory_order_acquire)) {
            return; // Успешно захватили в userspace!
        }
        
        // Медленный путь: нужно ждать
        while (true) {
            // В реальном futex здесь был бы системный вызов futex_wait()
            std::this_thread::yield(); // Имитация ожидания в ядре
            
            expected = 0;
            if (futex_word.compare_exchange_strong(expected, 1, std::memory_order_acquire)) {
                return;
            }
        }
    }
    
    void unlock() {
        futex_word.store(0, std::memory_order_release);
        // В реальном futex здесь был бы futex_wake() если есть ожидающие
    }
};

// Демонстрация оптимистической блокировки
class OptimisticLock {
private:
    std::atomic<int> lock_state{0};
    
public:
    bool try_lock_fast() {
        int expected = 0;
        return lock_state.compare_exchange_strong(expected, 1, std::memory_order_acquire);
    }
    
    void lock_slow() {
        // Spinning с экспоненциальной задержкой
        int spin_count = 0;
        while (!try_lock_fast()) {
            if (spin_count < 1000) {
                // Короткое spinning
                for (int i = 0; i < (1 << spin_count); ++i) {
                    std::this_thread::yield();
                }
                spin_count++;
            } else {
                // Длительное ожидание - уступаем
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }
    }
    
    void unlock() {
        lock_state.store(0, std::memory_order_release);
    }
};

void demo_futex_principles() {
    std::cout << "\n=== ДЕМО: Принципы Futex ===\n";
    
    SimpleFutex simple_lock;
    int shared_resource = 0;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < 1000; ++j) {
                simple_lock.lock();
                ++shared_resource; // Критическая секция
                simple_lock.unlock();
            }
            std::cout << "Поток " << i << " завершен\n";
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "Финальное значение ресурса: " << shared_resource << "\n";
    std::cout << "Ожидаемое значение: " << (4 * 1000) << "\n";
}

int main() {
    demo_futex_principles();
    return 0;
}
