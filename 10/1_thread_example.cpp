#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

void worker_function(int id) {
    std::cout << "Thread " << id << " starting\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "Thread " << id << " finished\n";
}

void demonstrate_basic_threads() {
    std::cout << "Hardware concurrency: " << std::thread::hardware_concurrency() << std::endl;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(worker_function, i);
    }
    
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void demonstrate_lambda_threads() {
    auto lambda_worker = [](int start, int end) {
        for (int i = start; i < end; ++i) {
            std::cout << "Processing " << i << " on thread " 
                      << std::this_thread::get_id() << std::endl;
        }
    };
    
    std::thread t1(lambda_worker, 0, 5);
    std::thread t2(lambda_worker, 5, 10);
    
    t1.join();
    t2.join();
}

int shared_counter = 0;

void increment_counter(int iterations) {
    for (int i = 0; i < iterations; ++i) {
        shared_counter++; // Небезопасная операция!
    }
}

void demonstrate_race_condition() {
    const int iterations = 100000;
    
    std::thread t1(increment_counter, iterations);
    std::thread t2(increment_counter, iterations);
    
    t1.join();
    t2.join();
    
    std::cout << "Expected: " << 2 * iterations << std::endl;
    std::cout << "Actual: " << shared_counter << std::endl;
}

int main() {
    std::cout << "=== Basic Threads Demo ===" << std::endl;
    demonstrate_basic_threads();
    
    std::cout << "\n=== Lambda Threads Demo ===" << std::endl;
    demonstrate_lambda_threads();
    
    std::cout << "\n=== Race Condition Demo ===" << std::endl;
    demonstrate_race_condition();
    
    return 0;
}
