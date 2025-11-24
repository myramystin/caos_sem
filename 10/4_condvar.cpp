#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <chrono>
#include <random>

class ProducerConsumer {
private:
    std::queue<int> buffer_;
    const size_t max_size_;
    mutable std::mutex mtx_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    bool finished_;

public:
    ProducerConsumer(size_t max_size) 
        : max_size_(max_size), finished_(false) {}
    
    void produce(int item) {
        std::unique_lock<std::mutex> lock(mtx_);
        
        // Ждем пока буфер не освободится
        not_full_.wait(lock, [this]() { 
            return buffer_.size() < max_size_; 
        });
        
        buffer_.push(item);
        std::cout << "Produced: " << item << " (buffer size: " 
                  << buffer_.size() << ")" << std::endl;
        
        not_empty_.notify_one();
    }
    
    bool consume(int& item) {
        std::unique_lock<std::mutex> lock(mtx_);
        
        // Ждем пока буфер не заполнится или производство не закончится
        not_empty_.wait(lock, [this]() { 
            return !buffer_.empty() || finished_; 
        });
        
        if (buffer_.empty() && finished_) {
            return false; // Больше нет элементов
        }
        
        item = buffer_.front();
        buffer_.pop();
        std::cout << "Consumed: " << item << " (buffer size: " 
                  << buffer_.size() << ")" << std::endl;
        
        not_full_.notify_one();
        return true;
    }
    
    void finish() {
        std::lock_guard<std::mutex> lock(mtx_);
        finished_ = true;
        not_empty_.notify_all();
    }
};

// Barrier - синхронизация группы потоков
class Barrier {
private:
    std::mutex mtx_;
    std::condition_variable cv_;
    const size_t thread_count_;
    size_t waiting_count_;
    size_t generation_;

public:
    explicit Barrier(size_t count) 
        : thread_count_(count), waiting_count_(0), generation_(0) {}
    
    void wait() {
        std::unique_lock<std::mutex> lock(mtx_);
        size_t current_generation = generation_;
        
        if (++waiting_count_ == thread_count_) {
            // Последний поток - разбудить всех
            waiting_count_ = 0;
            generation_++;
            cv_.notify_all();
        } else {
            // Ждать пока все потоки не достигнут барьера
            cv_.wait(lock, [this, current_generation]() {
                return current_generation != generation_;
            });
        }
    }
};

void demonstrate_producer_consumer() {
    std::cout << "=== Producer-Consumer Demo ===" << std::endl;
    
    ProducerConsumer pc(3); // Буфер размером 3
    
    // Producer thread
    std::thread producer([&pc]() {
        for (int i = 1; i <= 10; ++i) {
            pc.produce(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        pc.finish();
        std::cout << "Producer finished" << std::endl;
    });
    
    // Consumer threads
    std::vector<std::thread> consumers;
    for (int i = 0; i < 2; ++i) {
        consumers.emplace_back([&pc, i]() {
            int item;
            while (pc.consume(item)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
            std::cout << "Consumer " << i << " finished" << std::endl;
        });
    }
    
    producer.join();
    for (auto& consumer : consumers) {
        consumer.join();
    }
}

void demonstrate_barrier() {
    std::cout << "\n=== Barrier Demo ===" << std::endl;
    
    const int num_threads = 4;
    Barrier barrier(num_threads);
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&barrier, i]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(100, 1000);
            
            // Фаза 1
            int work_time = dis(gen);
            std::cout << "Thread " << i << " working for " << work_time << "ms" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(work_time));
            std::cout << "Thread " << i << " finished phase 1" << std::endl;
            
            barrier.wait(); // Ждем все потоки
            
            // Фаза 2 - все потоки начинают одновременно
            std::cout << "Thread " << i << " starting phase 2" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            std::cout << "Thread " << i << " finished phase 2" << std::endl;
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

void demonstrate_timeout() {
    std::cout << "\n=== Timeout Demo ===" << std::endl;
    
    std::mutex mtx;
    std::condition_variable cv;
    bool data_ready = false;
    
    std::thread worker([&]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        {
            std::lock_guard<std::mutex> lock(mtx);
            data_ready = true;
        }
        cv.notify_one();
        std::cout << "Worker: data is ready" << std::endl;
    });
    
    // Ждем с timeout
    {
        std::unique_lock<std::mutex> lock(mtx);
        if (cv.wait_for(lock, std::chrono::seconds(1), [&]() { return data_ready; })) {
            std::cout << "Main: got data within timeout" << std::endl;
        } else {
            std::cout << "Main: timeout occurred" << std::endl;
        }
        
        // Ждем еще раз без timeout
        cv.wait(lock, [&]() { return data_ready; });
        std::cout << "Main: finally got data" << std::endl;
    }
    
    worker.join();
}

int main() {
    demonstrate_producer_consumer();
    demonstrate_barrier();
    demonstrate_timeout();
    
    return 0;
}
