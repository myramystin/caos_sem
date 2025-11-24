#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>

class SafeCounter {
private:
    mutable std::mutex mtx_;
    int value_;

public:
    SafeCounter() : value_(0) {}
    
    void increment() {
        std::lock_guard<std::mutex> lock(mtx_);
        ++value_;
    }
    
    void decrement() {
        std::lock_guard<std::mutex> lock(mtx_);
        --value_;
    }
    
    int get() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return value_;
    }
    
    void unsafe_increment() {
        ++value_;
    }
};

int unsafe_counter = 0;
std::mutex counter_mutex;

void unsafe_worker(int iterations) {
    for (int i = 0; i < iterations; ++i) {
        unsafe_counter++; // DATA RACE!
    }
}

void safe_worker(int iterations) {
    for (int i = 0; i < iterations; ++i) {
        std::lock_guard<std::mutex> lock(counter_mutex);
        unsafe_counter++;
    }
}

void demonstrate_data_race() {
    const int iterations = 100000;
    const int num_threads = 4;
    
    std::cout << "=== Unsafe increment ===" << std::endl;
    unsafe_counter = 0;
    
    std::vector<std::thread> threads;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(unsafe_worker, iterations);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Expected: " << num_threads * iterations << std::endl;
    std::cout << "Actual: " << unsafe_counter << std::endl;
    std::cout << "Time: " << duration.count() << " microseconds" << std::endl;
    
    std::cout << "\n=== Safe increment ===" << std::endl;
    unsafe_counter = 0;
    threads.clear();
    
    start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(safe_worker, iterations);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Expected: " << num_threads * iterations << std::endl;
    std::cout << "Actual: " << unsafe_counter << std::endl;
    std::cout << "Time: " << duration.count() << " microseconds" << std::endl;
}

// Пример deadlock)
std::mutex mutex1, mutex2;

void worker1() {
    std::lock_guard<std::mutex> lock1(mutex1);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::lock_guard<std::mutex> lock2(mutex2);
    std::cout << "Worker 1 completed" << std::endl;
}

void worker2() {
    std::lock_guard<std::mutex> lock2(mutex2);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::lock_guard<std::mutex> lock1(mutex1);
    std::cout << "Worker 2 completed" << std::endl;
}

void demonstrate_deadlock() {
    std::cout << "\n=== Deadlock Demo (will hang) ===" << std::endl;
    std::cout << "Starting threads that will deadlock..." << std::endl;
    
    std::thread t1(worker1);
    std::thread t2(worker2);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // В реальности здесь будет deadlock, поэтому мы детачим потоки
    t1.detach();
    t2.detach();
    
    std::cout << "Threads detached (deadlock avoided)" << std::endl;
}

int main() {
    demonstrate_data_race();
    
    std::cout << "\n=== SafeCounter Demo ===" << std::endl;
    SafeCounter counter;
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&counter]() {
            for (int j = 0; j < 1000; ++j) {
                counter.increment();
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "Final counter value: " << counter.get() << std::endl;
    
    demonstrate_deadlock();
    
    return 0;
}
