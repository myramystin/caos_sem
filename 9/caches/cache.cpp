#include <iostream>
#include <chrono>
#include <vector>
#include <random>

class MatrixOptimizations {
private:
    static const size_t BLOCK_SIZE = 64;  // Размер блока для блочного алгоритма
    
public:
    // Неоптимизированное умножение матриц (плохо для кеша)
    void multiply_naive(const std::vector<std::vector<double>>& A,
                       const std::vector<std::vector<double>>& B,
                       std::vector<std::vector<double>>& C) {
        size_t n = A.size();
        
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < n; ++j) {
                C[i][j] = 0;
                for (size_t k = 0; k < n; ++k) {
                    C[i][j] += A[i][k] * B[k][j];  // Плохая локальность для B
                }
            }
        }
    }
    
    // Оптимизированное умножение с переставленными циклами
    void multiply_optimized(const std::vector<std::vector<double>>& A,
                           const std::vector<std::vector<double>>& B,
                           std::vector<std::vector<double>>& C) {
        size_t n = A.size();
        
        // Инициализация
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < n; ++j) {
                C[i][j] = 0;
            }
        }
        
        // Переставленные циклы для лучшей локальности
        for (size_t i = 0; i < n; ++i) {
            for (size_t k = 0; k < n; ++k) {
                for (size_t j = 0; j < n; ++j) {
                    C[i][j] += A[i][k] * B[k][j];
                }
            }
        }
    }
    
    // Блочное умножение матриц
    void multiply_blocked(const std::vector<std::vector<double>>& A,
                         const std::vector<std::vector<double>>& B,
                         std::vector<std::vector<double>>& C) {
        size_t n = A.size();
        
        // Инициализация
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < n; ++j) {
                C[i][j] = 0;
            }
        }
        
        // Блочное умножение
        for (size_t ii = 0; ii < n; ii += BLOCK_SIZE) {
            for (size_t jj = 0; jj < n; jj += BLOCK_SIZE) {
                for (size_t kk = 0; kk < n; kk += BLOCK_SIZE) {
                    
                    // Умножение блоков
                    size_t i_end = std::min(ii + BLOCK_SIZE, n);
                    size_t j_end = std::min(jj + BLOCK_SIZE, n);
                    size_t k_end = std::min(kk + BLOCK_SIZE, n);
                    
                    for (size_t i = ii; i < i_end; ++i) {
                        for (size_t k = kk; k < k_end; ++k) {
                            for (size_t j = jj; j < j_end; ++j) {
                                C[i][j] += A[i][k] * B[k][j];
                            }
                        }
                    }
                }
            }
        }
    }
    
    void benchmark_algorithms(size_t n) {
        std::cout << "Matrix multiplication benchmark (size: " << n << "x" << n << ")" << std::endl;
        std::cout << "=================================================" << std::endl;
        
        // Инициализация матриц
        std::vector<std::vector<double>> A(n, std::vector<double>(n));
        std::vector<std::vector<double>> B(n, std::vector<double>(n));
        std::vector<std::vector<double>> C(n, std::vector<double>(n));
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dis(0.0, 1.0);
        
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < n; ++j) {
                A[i][j] = dis(gen);
                B[i][j] = dis(gen);
            }
        }
        
        // Тест наивного алгоритма
        {
            auto start = std::chrono::high_resolution_clock::now();
            multiply_naive(A, B, C);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            std::cout << "Naive algorithm: " << duration << " ms" << std::endl;
        }
        
        // Тест оптимизированного алгоритма
        {
            auto start = std::chrono::high_resolution_clock::now();
            multiply_optimized(A, B, C);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            std::cout << "Optimized algorithm: " << duration << " ms" << std::endl;
        }
        
        // Тест блочного алгоритма
        {
            auto start = std::chrono::high_resolution_clock::now();
            multiply_blocked(A, B, C);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            std::cout << "Blocked algorithm: " << duration << " ms" << std::endl;
        }
    }
};

int main() {
    MatrixOptimizations optimizer;
    
    // Тестируем на матрицах разного размера
    for (size_t n : {128, 256, 512}) {
        optimizer.benchmark_algorithms(n);
        std::cout << std::endl;
    }
    
    return 0;
}
