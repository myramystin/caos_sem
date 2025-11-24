#include <mutex>
#include <queue>
#include <atomic>
#include <thread>
#include <chrono>
#include <system_error>

class ConditionVariable {
private:
    
    std::queue<std::thread::id> waiting_threads;
    
    
    mutable std::mutex cv_mutex;
    
    
    std::atomic<bool> signaled{false};
    
    
    std::mutex notification_mutex;
    std::atomic<int> notification_counter{0};

public:
    ConditionVariable() = default;
    ~ConditionVariable() = default;
    
    
    ConditionVariable(const ConditionVariable&) = delete;
    ConditionVariable& operator=(const ConditionVariable&) = delete;

    
    template<typename Predicate>
    void wait(std::unique_lock<std::mutex>& lock, Predicate pred) {
        while (!pred()) {
            wait(lock);
        }
    }
    
    
    void wait(std::unique_lock<std::mutex>& lock) {
        if (!lock.owns_lock()) {
            throw std::system_error(std::make_error_code(std::errc::operation_not_permitted));
        }
        
        auto thread_id = std::this_thread::get_id();
        int current_notification;
        
        
        {
            std::lock_guard<std::mutex> cv_lock(cv_mutex);
            waiting_threads.push(thread_id);
            current_notification = notification_counter.load();
        }
        
        
        lock.unlock();
        
        
        while (current_notification == notification_counter.load()) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        
        lock.lock();
        
        
        {
            std::lock_guard<std::mutex> cv_lock(cv_mutex);
            std::queue<std::thread::id> temp_queue;
            while (!waiting_threads.empty()) {
                if (waiting_threads.front() != thread_id) {
                    temp_queue.push(waiting_threads.front());
                }
                waiting_threads.pop();
            }
            waiting_threads = std::move(temp_queue);
        }
    }
    
    
    template<typename Rep, typename Period, typename Predicate>
    bool wait_for(std::unique_lock<std::mutex>& lock,
                  const std::chrono::duration<Rep, Period>& timeout_duration,
                  Predicate pred) {
        auto start_time = std::chrono::steady_clock::now();
        
        while (!pred()) {
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed >= timeout_duration) {
                return false; 
            }
            
            auto remaining = timeout_duration - elapsed;
            if (!wait_for(lock, remaining)) {
                return pred(); 
            }
        }
        return true;
    }
    
    
    template<typename Rep, typename Period>
    bool wait_for(std::unique_lock<std::mutex>& lock,
                  const std::chrono::duration<Rep, Period>& timeout_duration) {
        if (!lock.owns_lock()) {
            throw std::system_error(std::make_error_code(std::errc::operation_not_permitted));
        }
        
        auto thread_id = std::this_thread::get_id();
        int current_notification;
        auto start_time = std::chrono::steady_clock::now();
        
        {
            std::lock_guard<std::mutex> cv_lock(cv_mutex);
            waiting_threads.push(thread_id);
            current_notification = notification_counter.load();
        }
        
        lock.unlock();
        
        bool notified = false;
        while (current_notification == notification_counter.load()) {
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed >= timeout_duration) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        notified = (current_notification != notification_counter.load());
        
        lock.lock();
        
        
        {
            std::lock_guard<std::mutex> cv_lock(cv_mutex);
            std::queue<std::thread::id> temp_queue;
            while (!waiting_threads.empty()) {
                if (waiting_threads.front() != thread_id) {
                    temp_queue.push(waiting_threads.front());
                }
                waiting_threads.pop();
            }
            waiting_threads = std::move(temp_queue);
        }
        
        return notified;
    }
    
    
    void notify_one() {
        std::lock_guard<std::mutex> cv_lock(cv_mutex);
        if (!waiting_threads.empty()) {
            notification_counter.fetch_add(1);
        }
    }
    
    
    void notify_all() {
        std::lock_guard<std::mutex> cv_lock(cv_mutex);
        if (!waiting_threads.empty()) {
            int waiting_count = waiting_threads.size();
            notification_counter.fetch_add(waiting_count);
        }
    }
    
    
    size_t waiting_count() const {
        std::lock_guard<std::mutex> cv_lock(cv_mutex);
        return waiting_threads.size();
    }
};
