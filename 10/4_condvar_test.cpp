#include <sys/resource.h> 
#include <vector>
#include <iostream>
#include <thread>

#include <chrono>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

class PerformanceTester {
private:
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> condition{false};
    std::atomic<int> cpu_waste_counter{0};

public:
    // ПЛОХОЙ подход: активное ожидание с mutex
    void active_waiting_bad() {
        auto start = std::chrono::high_resolution_clock::now();
        
        while (true) {
            {
                std::lock_guard<std::mutex> lock(mtx);
                if (condition.load()) {
                    break;
                }
            }
            cpu_waste_counter++; // Считаем "пустые" циклы
            // Поток постоянно "молотит" CPU
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Active waiting: " << duration.count() << " μs, "
                  << "CPU cycles wasted: " << cpu_waste_counter.load() << std::endl;
    }
    
    // ХОРОШИЙ подход: пассивное ожидание с condition_variable
    void passive_waiting_good() {
        auto start = std::chrono::high_resolution_clock::now();
        
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return condition.load(); });
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Passive waiting: " << duration.count() << " μs, "
                  << "CPU cycles wasted: 0" << std::endl;
    }
    
    void signal_condition() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Имитируем работу
        {
            std::lock_guard<std::mutex> lock(mtx);
            condition = true;
        }
        cv.notify_all();
    }
    
    void reset() {
        condition = false;
        cpu_waste_counter = 0;
    }
};


class CPUMonitor {
private:
    std::chrono::high_resolution_clock::time_point start_time;
    clock_t start_cpu;
    
public:
    void start_monitoring() {
        start_time = std::chrono::high_resolution_clock::now();
        start_cpu = clock();
    }
    
    void print_cpu_usage(const std::string& test_name) {
        auto end_time = std::chrono::high_resolution_clock::now();
        clock_t end_cpu = clock();
        
        auto wall_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time).count();
        
        double cpu_time = static_cast<double>(end_cpu - start_cpu) / CLOCKS_PER_SEC * 1000;
        double cpu_usage = (cpu_time / wall_time) * 100;
        
        std::cout << test_name << ":\n"
                  << "  Wall time: " << wall_time << " ms\n"
                  << "  CPU time: " << cpu_time << " ms\n" 
                  << "  CPU usage: " << cpu_usage << "%\n\n";
    }
};

void comprehensive_performance_test() {
    const int NUM_THREADS = 4;
    const int TEST_DURATION_MS = 1000;
    
    std::cout << "=== АКТИВНОЕ ОЖИДАНИЕ (MUTEX POLLING) ===\n";
    {
        CPUMonitor monitor;
        PerformanceTester tester;
        std::vector<std::thread> threads;
        
        monitor.start_monitoring();
        
        
        for (int i = 0; i < NUM_THREADS; ++i) {
            threads.emplace_back(&PerformanceTester::active_waiting_bad, &tester);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(TEST_DURATION_MS));
        tester.signal_condition();
        
        for (auto& t : threads) {
            t.join();
        }
        
        monitor.print_cpu_usage("Active waiting with mutex");
    }
    
    std::cout << "=== ПАССИВНОЕ ОЖИДАНИЕ (CONDITION_VARIABLE) ===\n";
    {
        CPUMonitor monitor;
        PerformanceTester tester;
        std::vector<std::thread> threads;
        
        monitor.start_monitoring();
        
        
        for (int i = 0; i < NUM_THREADS; ++i) {
            threads.emplace_back(&PerformanceTester::passive_waiting_good, &tester);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(TEST_DURATION_MS));
        tester.signal_condition();
        
        for (auto& t : threads) {
            t.join();
        }
        
        monitor.print_cpu_usage("Passive waiting with condition_variable");
    }
}

int main() {
    comprehensive_performance_test();
    return 0;
}
