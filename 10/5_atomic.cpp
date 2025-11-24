#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>

void compare_atomic_vs_regular() {
    std::cout << "=== Atomic vs Regular Counter ===" << std::endl;
    
    const int iterations = 1000000;
    const int num_threads = 4;
    
    int regular_counter = 0;
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&regular_counter, iterations]() {
            for (int j = 0; j < iterations; ++j) {
                regular_counter++; // DATA RACE!
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Regular counter - Expected: " << num_threads * iterations 
              << ", Actual: " << regular_counter 
              << ", Time: " << duration1.count() << " μs" << std::endl;
    
    std::atomic<int> atomic_counter{0};
    threads.clear();
    
    start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&atomic_counter, iterations]() {
            for (int j = 0; j < iterations; ++j) {
                atomic_counter++; // Атомарная операция
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    end = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Atomic counter - Expected: " << num_threads * iterations 
              << ", Actual: " << atomic_counter.load() 
              << ", Time: " << duration2.count() << " μs" << std::endl;
    
    std::cout << "Performance ratio: " << (double)duration2.count() / duration1.count() 
              << "x slower" << std::endl;
}

void demonstrate_atomic_operations() {
    std::cout << "\n=== Atomic Operations Demo ===" << std::endl;
    
    std::atomic<int> counter{10};
    
    std::cout << "Initial value: " << counter.load() << std::endl;
    
    // Основные операции
    counter.store(20);
    std::cout << "After store(20): " << counter.load() << std::endl;
    
    int old_value = counter.exchange(30);
    std::cout << "After exchange(30), old value was: " << old_value 
              << ", new value: " << counter.load() << std::endl;
    
    // Compare and swap
    int expected = 30;
    bool success = counter.compare_exchange_weak(expected, 40);
    std::cout << "CAS(30->40) success: " << success 
              << ", value: " << counter.load() << std::endl;
    
    expected = 30; // Сброшен после CAS
    success = counter.compare_exchange_weak(expected, 50);
    std::cout << "CAS(30->50) success: " << success 
              << ", expected after CAS: " << expected 
              << ", value: " << counter.load() << std::endl;
    
    // Арифметические операции
    int prev = counter.fetch_add(10);
    std::cout << "After fetch_add(10), prev: " << prev 
              << ", current: " << counter.load() << std::endl;
    
    prev = counter.fetch_sub(5);
    std::cout << "After fetch_sub(5), prev: " << prev 
              << ", current: " << counter.load() << std::endl;
    
    // Операторы
    std::cout << "Using operators: " << ++counter << std::endl;
    std::cout << "Using operators: " << counter-- << std::endl;
    std::cout << "Final value: " << counter.load() << std::endl;
}

int main() {
    std::cout << "Atomic is lock-free for int: " 
              << std::atomic<int>::is_always_lock_free << std::endl;
    
    compare_atomic_vs_regular();
    demonstrate_atomic_operations();
    
    return 0;
}
