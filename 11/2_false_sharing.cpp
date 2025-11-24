#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

// === ПРОБЛЕМА: False Sharing ===
struct BadCounters {
    int counter1;  // Эти счетчики в одной cache line!
    int counter2;  // При изменении одного, инвалидируется вся линия
    int counter3;
    int counter4;
};

// === РЕШЕНИЕ: Padding ===
struct alignas(64) GoodCounter {  // Выравнивание по размеру cache line
    int counter;
    char padding[60];  // Заполнение до 64 байт
};

// Альтернативное решение через alignas
struct AlignedCounters {
    alignas(64) int counter1;  // Каждый счетчик в своей cache line
    alignas(64) int counter2;
    alignas(64) int counter3;
    alignas(64) int counter4;
};

void benchmark_false_sharing() {
    std::cout << "\n=== ДЕМО: False Sharing ===\n";
    
    const int ITERATIONS = 10000000;
    
    // Тест с False Sharing
    {
        BadCounters bad_counters = {0, 0, 0, 0};
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        threads.emplace_back([&]() {
            for (int i = 0; i < ITERATIONS; ++i) {
                ++bad_counters.counter1;  // Поток 1 модифицирует counter1
            }
        });
        
        threads.emplace_back([&]() {
            for (int i = 0; i < ITERATIONS; ++i) {
                ++bad_counters.counter2;  // Поток 2 модифицирует counter2
            }
        });
        
        for (auto& t : threads) {
            t.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto bad_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "С False Sharing: " << bad_duration.count() << " мс\n";
        std::cout << "Результаты: " << bad_counters.counter1 << ", " << bad_counters.counter2 << "\n";
    }
    
    // Тест без False Sharing
    {
        AlignedCounters good_counters = {0, 0, 0, 0};
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        threads.emplace_back([&]() {
            for (int i = 0; i < ITERATIONS; ++i) {
                ++good_counters.counter1;  // В отдельной cache line
            }
        });
        
        threads.emplace_back([&]() {
            for (int i = 0; i < ITERATIONS; ++i) {
                ++good_counters.counter2;  // В отдельной cache line
            }
        });
        
        for (auto& t : threads) {
            t.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto good_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Без False Sharing: " << good_duration.count() << " мс\n";
        std::cout << "Результаты: " << good_counters.counter1 << ", " << good_counters.counter2 << "\n";
    }
}

int main() {
    benchmark_false_sharing();
    return 0;
}