#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <cstdlib>
#include <sys/wait.h>
#include <signal.h>
#include <vector>
#include <map>
#include <unistd.h>
#include <sys/types.h>

class WaitDemo {
public:
    void demonstrate_basic_wait() {
        std::cout << "\n=== Базовое использование wait ===" << std::endl;
        
        std::vector<pid_t> children;
        const int NUM_CHILDREN = 3;
        

        for (int i = 0; i < NUM_CHILDREN; ++i) {
            pid_t pid = fork();
            
            if (pid == 0) {
        
                int sleep_time = 2 + i;
                int exit_code = 100 + i;
                
                std::cout << "Дочерний " << i << " (PID=" << getpid() 
                          << ") работает " << sleep_time << " секунд" << std::endl;
                
                std::this_thread::sleep_for(std::chrono::seconds(sleep_time));
                
                std::cout << "Дочерний " << i << " завершается с кодом " << exit_code << std::endl;
                _exit(exit_code);
                
            } else if (pid > 0) {
                children.push_back(pid);
            } else {
                throw std::runtime_error("fork failed");
            }
        }
        

        std::cout << "Родитель ждёт завершения дочерних процессов..." << std::endl;
        
        int status;
        pid_t finished_pid;
        int completed = 0;
        
        while ((finished_pid = wait(&status)) > 0) {
            completed++;
            
            std::cout << "Процесс " << finished_pid << " завершился ";
            
            if (WIFEXITED(status)) {
                std::cout << "нормально с кодом " << WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                std::cout << "по сигналу " << WTERMSIG(status);
#ifdef WCOREDUMP
                if (WCOREDUMP(status)) {
                    std::cout << " (core dump создан)";
                }
#endif
            } else if (WIFSTOPPED(status)) {
                std::cout << "остановлен сигналом " << WSTOPSIG(status);
            } else if (WIFCONTINUED(status)) {
                std::cout << "возобновлён";
            }
            
            std::cout << std::endl;
        }
        
        std::cout << "Завершено процессов: " << completed << std::endl;
    }
    
    void demonstrate_waitpid_options() {
        std::cout << "\n=== Использование waitpid с опциями ===" << std::endl;
        
        pid_t child = fork();
        
        if (child == 0) {
    
            std::cout << "Дочерний процесс начал долгую работу..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            std::cout << "Дочерний процесс завершает работу" << std::endl;
            _exit(42);
            
        } else if (child > 0) {
    
            std::cout << "Родитель использует неблокирующее ожидание..." << std::endl;
            
            int status;
            int checks = 0;
            
            while (true) {
                pid_t result = waitpid(child, &status, WNOHANG);
                checks++;
                
                if (result == 0) {
            
                    std::cout << "Проверка " << checks << ": дочерний процесс всё ещё работает" << std::endl;
                    
            
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    
                } else if (result == child) {
            
                    std::cout << "Дочерний процесс завершён с кодом " << 
                                 WEXITSTATUS(status) << std::endl;
                    break;
                    
                } else {
                    perror("waitpid");
                    break;
                }
            }
            
            std::cout << "Всего проверок: " << checks << std::endl;
            
        } else {
            throw std::runtime_error("fork failed");
        }
    }
    
    void demonstrate_zombie_process() {
        std::cout << "\n=== Демонстрация зомби-процессов ===" << std::endl;
        
        pid_t zombie_child = fork();
        
        if (zombie_child == 0) {
    
            std::cout << "Дочерний процесс (будущий зомби) PID=" << getpid() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "Дочерний процесс завершается..." << std::endl;
            _exit(123);
            
        } else if (zombie_child > 0) {
    
            std::cout << "Родитель НЕ вызывает wait - создаём зомби" << std::endl;
            std::cout << "Дочерний PID: " << zombie_child << std::endl;
            
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
    
            std::string cmd = "ps -o pid,ppid,state,comm -p " + std::to_string(zombie_child);
            std::cout << "Состояние дочернего процесса:" << std::endl;
            system(cmd.c_str());
            
            std::cout << "\nЗомби-процесс существует 5 секунд..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            std::cout << "Теперь забираем статус зомби..." << std::endl;
            int status;
            pid_t result = wait(&status);
            
            if (result == zombie_child) {
                std::cout << "Зомби-процесс очищен, код выхода: " << 
                             WEXITSTATUS(status) << std::endl;
            }
            
    
            std::cout << "Проверяем что процесс исчез:" << std::endl;
            system(cmd.c_str());
            
        } else {
            throw std::runtime_error("fork failed");
        }
    }
    
    void demonstrate_sigchld_handling() {
        std::cout << "\n=== Асинхронная обработка через SIGCHLD ===" << std::endl;
        

        signal(SIGCHLD, [](int sig) {
            std::cout << "SIGCHLD получен!" << std::endl;
            
    
            int status;
            pid_t pid;
            
            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                std::cout << "  Завершился процесс " << pid;
                if (WIFEXITED(status)) {
                    std::cout << " с кодом " << WEXITSTATUS(status);
                }
                std::cout << std::endl;
            }
        });
        

        std::vector<pid_t> children;
        
        for (int i = 0; i < 3; ++i) {
            pid_t pid = fork();
            
            if (pid == 0) {
        
                int delay = 1 + (i * 2);
                std::cout << "Дочерний " << i << " (PID=" << getpid() 
                          << ") будет работать " << delay << " сек" << std::endl;
                
                std::this_thread::sleep_for(std::chrono::seconds(delay));
                _exit(i + 10);
                
            } else if (pid > 0) {
                children.push_back(pid);
            } else {
                throw std::runtime_error("fork failed");
            }
        }
        

        std::cout << "Родитель выполняет другую работу..." << std::endl;
        
        for (int i = 0; i < 8; ++i) {
            std::cout << "Работа родителя, итерация " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        std::cout << "Родитель завершил свою работу" << std::endl;
        

        signal(SIGCHLD, SIG_DFL);
    }
    
    void demonstrate_wait_specific_child() {
        std::cout << "\n=== Ожидание конкретного дочернего процесса ===" << std::endl;
        
        std::map<pid_t, std::string> child_names;
        

        for (int i = 0; i < 3; ++i) {
            pid_t pid = fork();
            
            if (pid == 0) {
        
                int work_time = 3 - i;
                std::this_thread::sleep_for(std::chrono::seconds(work_time));
                _exit(i);
                
            } else if (pid > 0) {
                child_names[pid] = "Child_" + std::to_string(i);
                std::cout << "Создан " << child_names[pid] << " PID=" << pid 
                          << " (время работы: " << (3-i) << " сек)" << std::endl;
            } else {
                throw std::runtime_error("fork failed");
            }
        }
        

        std::cout << "\nОжидаем завершения в порядке создания:" << std::endl;
        
        for (const auto& [pid, name] : child_names) {
            std::cout << "Ожидаем " << name << " (PID=" << pid << ")..." << std::endl;
            
            int status;
            pid_t result = waitpid(pid, &status, 0); // Блокирующее ожидание
            
            if (result == pid) {
                std::cout << "  " << name << " завершился с кодом " << 
                             WEXITSTATUS(status) << std::endl;
            } else {
                perror("waitpid");
            }
        }
        
        std::cout << "Все дочерние процессы обработаны в нужном порядке" << std::endl;
    }
    
    void demonstrate_waitpid_errors() {
        std::cout << "\n=== Демонстрация обработки ошибок waitpid ===" << std::endl;
        

        pid_t fake_pid = 99999;
        int status;
        
        std::cout << "Попытка дождаться несуществующий процесс " << fake_pid << std::endl;
        pid_t result = waitpid(fake_pid, &status, WNOHANG);
        
        if (result == -1) {
            perror("waitpid для несуществующего процесса");
        } else {
            std::cout << "Неожиданный результат: " << result << std::endl;
        }
        

        std::cout << "Попытка дождаться процесс init (PID=1)" << std::endl;
        result = waitpid(1, &status, WNOHANG);
        
        if (result == -1) {
            perror("waitpid для процесса init");
        } else {
            std::cout << "Неожиданный результат: " << result << std::endl;
        }
    }
};

int main() {
    try {
        std::cout << "=== Демонстрация системных вызовов wait/waitpid ===" << std::endl;
        
        WaitDemo demo;
        

        demo.demonstrate_basic_wait();
        

        demo.demonstrate_waitpid_options();
        

        demo.demonstrate_zombie_process();
        

        demo.demonstrate_sigchld_handling();
        

        demo.demonstrate_wait_specific_child();
        

        demo.demonstrate_waitpid_errors();
        
        std::cout << "\n=== Все демонстрации wait завершены ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
