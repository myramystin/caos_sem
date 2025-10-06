#include <iostream>
#include <unistd.h>
#include <string>
#include <cstring>
#include <sys/wait.h>

int main() {
    int pipefd[2];
    pid_t pid;
    const std::string message = "Привет от родительского процесса!";
    char buffer[100];
    
    if (pipe(pipefd) == -1) {
        std::cerr << "Ошибка создания pipe" << std::endl;
        return 1;
    }
    
    pid = fork();
    
    if (pid == -1) {
        std::cerr << "Ошибка fork()" << std::endl;
        return 1;
    }
    
    if (pid == 0) {
        close(pipefd[1]);
        
        ssize_t bytesRead = read(pipefd[0], buffer, sizeof(buffer));
        if (bytesRead > 0) {
            std::cout << "Дочерний процесс получил: " << buffer << std::endl;
        }
        
        close(pipefd[0]);
        return 0;
    } else {
        close(pipefd[0]);

        write(pipefd[1], message.c_str(), message.length() + 1);
        std::cout << "Родительский процесс отправил сообщение" << std::endl;
        
        close(pipefd[1]);
        waitpid(pid, nullptr, 0);
    }
    
    return 0;
}
