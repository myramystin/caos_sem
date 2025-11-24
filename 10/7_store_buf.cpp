#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

// Алгоритм Петерсона (не работает без правильного memory ordering)
class PetersonLock {
private:
    std::atomic<bool> flag0_{false};
    std::atomic<bool> flag1_{false};  
    std::atomic<int> turn_{0};

public:
    void lock(int thread_id) {
        if (thread_id == 0) {
            flag0_.store(true, std::memory_order_relaxed);
            turn_.store(1, std::memory_order_relaxed);
            while (flag1_.load(std::memory_order_relaxed) && 
                   turn_.load(std::memory_order_relaxed) == 1) {
                std::this_thread::yield();
            }
        } else {
            flag1_.store(true, std::memory_order_relaxed);
            turn_.store(0, std::memory_order_relaxed);
            while (flag0_.load(std::memory_order_relaxed) && 
                   turn_.load(std::memory_order_relaxed) == 0) {
                std::this_thread::yield();
            }
        }
    }
    
    void unlock(int thread_id) {
        if (thread_id == 0) {
            flag0_.store(false, std::memory_order_relaxed);
        } else {
            flag1_.store(false, std::memory_order_relaxed);
        }
    }
};

void demonstrate_peterson_lock() {
    std::cout << "\n=== Peterson Lock Demo (may fail due to memory reordering) ===" << std::endl;
    
    PetersonLock lock;
    int shared_counter = 0;
    const int iterations = 100000;
    
    std::thread t0([&]() {
        for (int i = 0; i < iterations; ++i) {
            lock.lock(0);
            shared_counter++; // Critical section
            lock.unlock(0);
        }
    });
    
    std::thread t1([&]() {
        for (int i = 0; i < iterations; ++i) {
            lock.lock(1);
            shared_counter++; // Critical section
            lock.unlock(1);
        }
    });
    
    t0.join();
    t1.join();
    
    std::cout << "Expected: " << 2 * iterations << std::endl;
    std::cout << "Actual: " << shared_counter << std::endl;
    if (shared_counter != 2 * iterations) {
        std::cout << "Peterson lock failed due to memory reordering!" << std::endl;
    } else {
        std::cout << "Peterson lock worked (lucky this time)" << std::endl;
    }
}

int main() {
    demonstrate_peterson_lock();
    return 0;
}
