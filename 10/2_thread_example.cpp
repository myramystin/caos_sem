#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

#include <pthread.h>
#include <sched.h>


std::atomic<int> counter{0};
const int ITERATIONS = 1000000;

void increment_worker() {
    for (int i = 0; i < ITERATIONS; ++i) {
        counter.fetch_add(1, std::memory_order_relaxed);
    }
}

void set_thread_affinity(std::thread& t, int core) {
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);
    pthread_setaffinity_np(t.native_handle(), sizeof(cpu_set_t), &cpuset);
#endif
}

int main() {
    auto start = std::chrono::high_resolution_clock::now();
    
    std::thread t1(increment_worker);
    std::thread t2(increment_worker);
    
    t1.join();
    t2.join();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Without affinity - Counter: " << counter.load() 
              << ", Time: " << duration.count() << "ms" << std::endl;
    
    counter.store(0);
    
    start = std::chrono::high_resolution_clock::now();
    
    std::thread t3(increment_worker);
    std::thread t4(increment_worker);
    
    set_thread_affinity(t3, 0);
    set_thread_affinity(t4, 0);
    
    t3.join();
    t4.join();
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "With same core affinity - Counter: " << counter.load() 
              << ", Time: " << duration.count() << "ms" << std::endl;
    
    return 0;
}
