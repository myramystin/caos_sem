#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <unistd.h>

std::vector<int> numbers = {10, 20, 30, 40, 50};
int counter = 0;

int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

void process_data() {
    while (true) {
        counter++;
        
        for (size_t i = 0; i < numbers.size(); i++) {
            numbers[i] = numbers[i] + counter;
        }
        
        int fact = factorial(counter % 10);
        
        std::cout << "Iteration " << counter << ": ";
        std::cout << "Factorial(" << counter % 10 << ") = " << fact << std::endl;
        std::cout << "Numbers: ";
        for (int num : numbers) {
            std::cout << num << " ";
        }
        std::cout << std::endl << std::endl;
    
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

int main() {
    std::cout << "Starting simple program. PID: " << getpid() << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl << std::endl;
    
    process_data();
    
    return 0;
}
