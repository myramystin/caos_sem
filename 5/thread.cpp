#include <iostream>
#include <thread>
#include <unistd.h>
#include <mutex>

int sharedVar = 10;
std::mutex mtx;

void threadFunction() {
    sharedVar += 15;
    
    {
        std::lock_guard<std::mutex> lock(mtx);
        std::cout << "Поток: sharedVar = " << sharedVar 
                  << " (Thread ID: " << std::this_thread::get_id() << ")" << std::endl;
    }
}

int main() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        std::cout << "Начальное значение: " << sharedVar << std::endl;
    }
    
    std::thread t(threadFunction);
    t.join();
    
    {
        std::lock_guard<std::mutex> lock(mtx);
        std::cout << "Основной поток: sharedVar = " << sharedVar 
                  << " (PID: " << getpid() << ")" << std::endl;
    }
    
    return 0;
}
