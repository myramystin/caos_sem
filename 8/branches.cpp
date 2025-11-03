#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <random>

const int ARRAY_SIZE = 32768;
const int ITERATIONS = 100000;

template<typename Func>
double measure_time(Func func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

// Тест с предсказуемыми ветвлениями (отсортированный массив)
long long test_predictable(const std::vector<int>& data) {
    long long sum = 0;
    for (int iteration = 0; iteration < ITERATIONS; ++iteration) {
        for (int i = 0; i < ARRAY_SIZE; ++i) {
            // Предсказуемое ветвление: сначала все false, потом все true
            if (data[i] >= ARRAY_SIZE / 2) {
                sum += data[i];
            }
        }
    }
    return sum;
}

// Тест с непредсказуемыми ветвлениями (случайный массив)  
long long test_unpredictable(const std::vector<int>& data) {
    long long sum = 0;
    for (int iteration = 0; iteration < ITERATIONS; ++iteration) {
        for (int i = 0; i < ARRAY_SIZE; ++i) {
            // Непредсказуемое ветвление: случайный порядок
            if (data[i] >= ARRAY_SIZE / 2) {
                sum += data[i];
            }
        }
    }
    return sum;
}

// Версия без ветвлений (branchless)
long long test_branchless(const std::vector<int>& data) {
    long long sum = 0;
    for (int iteration = 0; iteration < ITERATIONS; ++iteration) {
        for (int i = 0; i < ARRAY_SIZE; ++i) {
            // Используем тернарный оператор и надеемся на cmov
            sum += (data[i] >= ARRAY_SIZE / 2) ? data[i] : 0;
        }
    }
    return sum;
}

int main() {
    std::vector<int> sorted_data(ARRAY_SIZE);
    std::vector<int> random_data(ARRAY_SIZE);
    
    for (int i = 0; i < ARRAY_SIZE; ++i) {
        sorted_data[i] = i;
        random_data[i] = i;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(random_data.begin(), random_data.end(), gen);
    
    std::cout << "Branch Prediction Demo\n";
    std::cout << "Array size: " << ARRAY_SIZE << ", Iterations: " << ITERATIONS << "\n\n";
    
    double predictable_time = measure_time([&]() {
        test_predictable(sorted_data);
    });
    
    double unpredictable_time = measure_time([&]() {
        test_unpredictable(random_data);
    });
    
    double branchless_time = measure_time([&]() {
        test_branchless(random_data);
    });
    
    std::cout << "Predictable branches (sorted):   " << predictable_time << " ms\n";
    std::cout << "Unpredictable branches (random): " << unpredictable_time << " ms\n"; 
    std::cout << "Branchless version:              " << branchless_time << " ms\n\n";
    
    std::cout << "Slowdown from misprediction: " 
              << (unpredictable_time / predictable_time) << "x\n";
              
    
    return 0;
}
