#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int main() {
    int sharedVar = 10;
    
    std::cout << "Начальное значение: " << sharedVar << std::endl;
    
    pid_t pid = fork();
    
    if (pid < 0) {
        std::cerr << "Ошибка fork()" << std::endl;
        return 1;
    } else if (pid == 0) {
        sharedVar += 15;
        std::cout << "Дочерний процесс: sharedVar = " << sharedVar 
                  << " (PID: " << getpid() << ")" << std::endl;
        return 0;
    } else {
        int status;
        waitpid(pid, &status, 0);
        std::cout << "Родительский процесс: sharedVar = " << sharedVar 
                  << " (PID: " << getpid() << ")" << std::endl;
    }
    
    return 0;
}
