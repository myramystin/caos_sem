#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <functional>
#include <stdexcept>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>

class ExecDemo {
public:
    void demonstrate_basic_exec() {
        std::cout << "\n=== Базовое использование exec ===" << std::endl;
        
        pid_t pid = fork();
        
        if (pid == 0) {
            std::cout << "Дочерний процесс до exec: PID=" << getpid() << std::endl;
            
            execl("/bin/ls", "ls", "-la", "/tmp", nullptr);
            
            // Эта строка никогда не выполнится, если exec успешен
            std::cerr << "ОШИБКА: exec не должен возвращать управление!" << std::endl;
            perror("execl failed");
            _exit(127);
            
        } else if (pid > 0) {
            int status;
            wait(&status);
            
            if (WIFEXITED(status)) {
                std::cout << "Дочерний процесс завершён с кодом " << 
                             WEXITSTATUS(status) << std::endl;
            }
        } else {
            throw std::runtime_error("fork failed");
        }
    }
    
    void demonstrate_exec_variants() {
        std::cout << "\n=== Варианты семейства exec ===" << std::endl;
        
        std::cout << "1. Используем execl:" << std::endl;
        run_exec_example([]{
            execl("/bin/echo", "echo", "Hello from execl!", nullptr);
        });
        
        std::cout << "\n2. Используем execv:" << std::endl;
        run_exec_example([]{
            char* argv[] = {
                const_cast<char*>("echo"),
                const_cast<char*>("Hello from execv!"),
                nullptr
            };
            execv("/bin/echo", argv);
        });
        
        std::cout << "\n3. Используем execlp (поиск в PATH):" << std::endl;
        run_exec_example([]{
            execlp("echo", "echo", "Hello from execlp!", nullptr);
        });
        
        std::cout << "\n4. Используем execvp:" << std::endl;
        run_exec_example([]{
            char* argv[] = {
                const_cast<char*>("echo"),
                const_cast<char*>("Hello from execvp!"),
                nullptr
            };
            execvp("echo", argv);
        });
    }
    
    void demonstrate_exec_with_environment() {
        std::cout << "\n=== Exec с custom environment ===" << std::endl;
        
        std::vector<std::string> env_strings = {
            "PATH=/bin:/usr/bin",
            "CUSTOM_VAR=Hello from custom env!",
            "USER=exec_demo",
            "HOME=/tmp"
        };
        
        std::vector<char*> envp;
        for (auto& env_str : env_strings) {
            envp.push_back(const_cast<char*>(env_str.c_str()));
        }
        envp.push_back(nullptr);
        
        pid_t pid = fork();
        
        if (pid == 0) {
            std::cout << "Запускаем программу с custom environment" << std::endl;
            
            char* argv[] = {
                const_cast<char*>("sh"),
                const_cast<char*>("-c"),
                const_cast<char*>("echo \"Custom var: $CUSTOM_VAR\"; echo \"User: $USER\"; env | grep CUSTOM"),
                nullptr
            };
            
            execve("/bin/sh", argv, envp.data());
            
            perror("execve failed");
            _exit(127);
            
        } else if (pid > 0) {
            wait(nullptr);
        } else {
            throw std::runtime_error("fork failed");
        }
    }
    
    void demonstrate_fd_across_exec() {
        std::cout << "\n=== Поведение файловых дескрипторов через exec ===" << std::endl;
        
        int fd = open("exec_fd_test.txt", O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd == -1) {
            throw std::runtime_error("open failed");
        }
        
        write(fd, "Before exec\n", 12);
        
        std::cout << "Файл открыт с дескриптором: " << fd << std::endl;
        
        pid_t pid1 = fork();
        if (pid1 == 0) {
            std::cout << "ТЕСТ 1: Дескриптор без FD_CLOEXEC" << std::endl;
            
            std::string fd_str = std::to_string(fd);
            std::string command = "echo 'After exec (inherited fd)' >&" + fd_str;
            
            execlp("sh", "sh", "-c", command.c_str(), nullptr);
            _exit(127);
        }
        
        wait(nullptr);
        
        int flags = fcntl(fd, F_GETFD);
        fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
        
        pid_t pid2 = fork();
        if (pid2 == 0) {
            std::cout << "ТЕСТ 2: Дескриптор с FD_CLOEXEC" << std::endl;
            
            std::string fd_str = std::to_string(fd);
            std::string command = "echo 'This should fail' >&" + fd_str + " 2>/dev/null || echo 'FD closed as expected'";
            
            execlp("sh", "sh", "-c", command.c_str(), nullptr);
            _exit(127);
        }
        
        wait(nullptr);
        close(fd);
        
        std::cout << "Содержимое файла:" << std::endl;
        system("cat exec_fd_test.txt");
    }
    
    void demonstrate_exec_errors() {
        std::cout << "\n=== Обработка ошибок exec ===" << std::endl;
        
        struct ExecTest {
            std::string description;
            std::function<void()> exec_call;
        };
        
        std::vector<ExecTest> tests = {
            {
                "Несуществующий файл",
                []() { execl("/nonexistent/program", "program", nullptr); }
            },
            {
                "Нет прав на выполнение",
                []() { 
                    system("echo '#!/bin/sh' > no_exec_perm.sh");
                    system("chmod 644 no_exec_perm.sh"); // Убираем +x
                    execl("./no_exec_perm.sh", "no_exec_perm.sh", nullptr);
                }
            },
            {
                "Неправильный формат файла",
                []() {
                    system("echo 'This is not an executable' > bad_format");
                    system("chmod +x bad_format");
                    execl("./bad_format", "bad_format", nullptr);
                }
            }
        };
        
        for (const auto& test : tests) {
            std::cout << "\nТест: " << test.description << std::endl;
            
            pid_t pid = fork();
            if (pid == 0) {
                test.exec_call();
                
                int error = errno;
                std::cout << "exec failed: " << strerror(error) << std::endl;
                _exit(127);
                
            } else if (pid > 0) {
                int status;
                wait(&status);
                
                if (WIFEXITED(status) && WEXITSTATUS(status) == 127) {
                    std::cout << "Ошибка exec обработана корректно" << std::endl;
                }
            }
        }
        
        system("rm -f no_exec_perm.sh bad_format");
    }
    
    void demonstrate_simple_shell() {
        std::cout << "\n=== Простая оболочка (shell) ===" << std::endl;
        std::cout << "Введите команды (или 'exit' для выхода):" << std::endl;
        
        std::string input;
        while (true) {
            std::cout << "myshell> ";
            std::cout.flush();
            
            if (!std::getline(std::cin, input) || input == "exit") {
                break;
            }
            
            if (input.empty()) continue;
            
            std::vector<std::string> tokens = parse_command(input);
            if (tokens.empty()) continue;
            
            if (tokens[0] == "cd") {
                if (tokens.size() > 1) {
                    if (chdir(tokens[1].c_str()) == -1) {
                        perror("cd");
                    }
                } else {
                    const char* home = getenv("HOME");
                    if (chdir(home ? home : "/") == -1) {
                        perror("cd");
                    }
                }
                continue;
            }
            
            execute_command(tokens);
        }
        
        std::cout << "Shell завершён" << std::endl;
    }
    
private:
    template<typename ExecFunc>
    void run_exec_example(ExecFunc exec_func) {
        pid_t pid = fork();
        
        if (pid == 0) {
            exec_func();
            perror("exec failed");
            _exit(127);
        } else if (pid > 0) {
            wait(nullptr);
        } else {
            throw std::runtime_error("fork failed");
        }
    }
    
    std::vector<std::string> parse_command(const std::string& input) {
        std::vector<std::string> tokens;
        std::istringstream iss(input);
        std::string token;
        
        while (iss >> token) {
            tokens.push_back(token);
        }
        
        return tokens;
    }
    
    void execute_command(const std::vector<std::string>& tokens) {
        std::vector<char*> argv;
        for (const auto& token : tokens) {
            argv.push_back(const_cast<char*>(token.c_str()));
        }
        argv.push_back(nullptr);
        
        pid_t pid = fork();
        
        if (pid == 0) {
            execvp(argv[0], argv.data());
            perror("Command not found");
            _exit(127);
            
        } else if (pid > 0) {
            int status;
            wait(&status);
            
            if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);
                if (exit_code != 0) {
                    std::cout << "Command exited with code: " << exit_code << std::endl;
                }
            } else if (WIFSIGNALED(status)) {
                std::cout << "Command terminated by signal: " << WTERMSIG(status) << std::endl;
            }
        } else {
            perror("fork failed");
        }
    }
};

int main() {
    try {
        ExecDemo demo;
        
        demo.demonstrate_basic_exec();
        demo.demonstrate_exec_variants();
        demo.demonstrate_exec_with_environment();
        demo.demonstrate_fd_across_exec();
        demo.demonstrate_exec_errors();
        
        // demo.demonstrate_simple_shell();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
