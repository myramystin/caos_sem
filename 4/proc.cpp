#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <sys/wait.h>
#include <signal.h>
#include <fstream>
#include <thread>
#include <unistd.h>
#include <sys/types.h>

class ProcessStateDemo {
private:
    enum class ProcessState {
        RUNNING = 'R',
        SLEEPING_INTERRUPTIBLE = 'S', 
        SLEEPING_UNINTERRUPTIBLE = 'D',
        STOPPED = 'T',
        TRACED = 't',
        ZOMBIE = 'Z',
        DEAD = 'X'
    };
    
    static std::string get_process_state(pid_t pid) {
        std::ifstream stat_file("/proc/" + std::to_string(pid) + "/stat");
        std::string line;
        if (std::getline(stat_file, line)) {
            std::istringstream iss(line);
            std::string field;
            
            for (int i = 0; i < 2; ++i) {
                iss >> field;
            }
            
            iss >> field;
            return field;
        }
        return "?";
    }
    
    static std::string state_description(char state) {
        switch (state) {
            case 'R': return "RUNNING (выполняется или готов к выполнению)";
            case 'S': return "SLEEPING INTERRUPTIBLE (прерываемый сон)";
            case 'D': return "SLEEPING UNINTERRUPTIBLE (непрерываемый сон)";
            case 'T': return "STOPPED (остановлен сигналом)";
            case 't': return "TRACED (трассируется отладчиком)";
            case 'Z': return "ZOMBIE (завершён, ждёт родителя)";
            case 'X': return "DEAD (удаляется)";
            default: return "UNKNOWN";
        }
    }
    
public:
    void demonstrate_process_states() {
        std::cout << "\n=== Демонстрация состояний процесса ===" << std::endl;
        
        pid_t pid = fork();
        
        if (pid == 0) {
            
            std::cout << "Дочерний процесс PID: " << getpid() << std::endl;
            
            
            std::string state = get_process_state(getpid());
            std::cout << "Состояние дочернего: " << state << " - " << 
                         state_description(state[0]) << std::endl;
            
            
            std::cout << "Переходим в состояние INTERRUPTIBLE (sleep)" << std::endl;
            sleep(30);  
            
            std::cout << "Дочерний процесс завершается" << std::endl;
            _exit(0);
            
        } else if (pid > 0) {
            
            std::cout << "Родительский процесс, дочерний PID: " << pid << std::endl;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            
            std::string state = get_process_state(pid);
            std::cout << "Состояние дочернего в sleep: " << state << " - " << 
                         state_description(state[0]) << std::endl;
            
            
            std::cout << "Посылаем SIGSTOP..." << std::endl;
            kill(pid, SIGSTOP);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            state = get_process_state(pid);
            std::cout << "После SIGSTOP: " << state << " - " << 
                         state_description(state[0]) << std::endl;
            
            
            std::cout << "Посылаем SIGCONT..." << std::endl;
            kill(pid, SIGCONT);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            state = get_process_state(pid);
            std::cout << "После SIGCONT: " << state << " - " << 
                         state_description(state[0]) << std::endl;
            
            
            std::cout << "Завершаем дочерний процесс..." << std::endl;
            kill(pid, SIGTERM);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            
            state = get_process_state(pid);
            std::cout << "После SIGTERM (должен быть ZOMBIE): " << state << " - " << 
                         state_description(state[0]) << std::endl;
            
            
            int status;
            wait(&status);
            std::cout << "После wait() процесс удален из таблицы процессов" << std::endl;
            
            
            state = get_process_state(pid);
            if (state == "?") {
                std::cout << "Процесс больше не существует" << std::endl;
            }
            
        } else {
            perror("fork failed");
        }
    }
    
    void demonstrate_process_tree() {
        std::cout << "\n=== Иерархия процессов ===" << std::endl;
        
        
        std::cout << "Текущий процесс:" << std::endl;
        std::cout << "  PID: " << getpid() << std::endl;
        std::cout << "  PPID: " << getppid() << std::endl;
        std::cout << "  PGID: " << getpgid(0) << std::endl;
        std::cout << "  SID: " << getsid(0) << std::endl;
        
        
        pid_t child1 = fork();
        if (child1 == 0) {
            std::cout << "\nПервый дочерний:" << std::endl;
            std::cout << "  PID: " << getpid() << std::endl;
            std::cout << "  PPID: " << getppid() << std::endl;
            
            
            pid_t grandchild = fork();
            if (grandchild == 0) {
                std::cout << "\nВнук:" << std::endl;
                std::cout << "  PID: " << getpid() << std::endl;
                std::cout << "  PPID: " << getppid() << std::endl;
                sleep(2);
                _exit(0);
            } else if (grandchild > 0) {
                wait(nullptr); 
                _exit(0);
            } else {
                perror("fork grandchild failed");
                _exit(1);
            }
        } else if (child1 < 0) {
            perror("fork child1 failed");
            return;
        }
        
        pid_t child2 = fork();
        if (child2 == 0) {
            std::cout << "\nВторой дочерний:" << std::endl;
            std::cout << "  PID: " << getpid() << std::endl;
            std::cout << "  PPID: " << getppid() << std::endl;
            sleep(1);
            _exit(0);
        } else if (child2 < 0) {
            perror("fork child2 failed");
            
            wait(nullptr);
            return;
        }
        
        
        wait(nullptr);
        wait(nullptr);
        
        std::cout << "\nВсе дочерние процессы завершены" << std::endl;
    }
    
    void demonstrate_process_info() {
        std::cout << "\n=== Подробная информация о процессе ===" << std::endl;
        
        pid_t current_pid = getpid();
        
        
        std::ifstream status_file("/proc/self/status");
        std::string line;
        
        std::cout << "Информация из /proc/self/status:" << std::endl;
        
        while (std::getline(status_file, line)) {
            
            if (line.substr(0, 4) == "Name" || 
                line.substr(0, 4) == "Pid:" ||
                line.substr(0, 4) == "PPid" ||
                line.substr(0, 5) == "State" ||
                line.substr(0, 6) == "Threads" ||
                line.substr(0, 5) == "VmPeak" ||
                line.substr(0, 6) == "VmSize" ||
                line.substr(0, 5) == "VmRSS") {
                std::cout << "  " << line << std::endl;
            }
        }
    }
};

int main() {
    try {
        std::cout << "=== Демонстрация состояний и свойств процессов ===" << std::endl;
        
        ProcessStateDemo demo;
        
        
        demo.demonstrate_process_info();
        
        
        demo.demonstrate_process_states();
        
        
        demo.demonstrate_process_tree();
        
        std::cout << "\n=== Все демонстрации процессов завершены ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
