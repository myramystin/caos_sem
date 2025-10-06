#include <iostream>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

class ExitDemo {
private:
    static std::ofstream* logfile;
    static bool cleanup_called;
    
    static void cleanup_function() {
        cleanup_called = true;
        std::cout << "cleanup_function() вызвана" << std::endl;
        
        if (logfile && logfile->is_open()) {
            *logfile << "Программа завершается через exit()" << std::endl;
            logfile->close();
            delete logfile;
            logfile = nullptr;
        }
    }
    
    static void another_cleanup() {
        std::cout << "another_cleanup() вызвана" << std::endl;
    }
    
public:
    void demonstrate_exit_vs_Exit() {
        std::cout << "\n=== Сравнение exit() и _Exit() ===" << std::endl;
        
        logfile = new std::ofstream("exit_log.txt");
        if (!logfile->is_open()) {
            throw std::runtime_error("Cannot open log file");
        }
        
        std::atexit(cleanup_function);
        std::atexit(another_cleanup);
        
        *logfile << "Данные в файловом буфере (не flushed)" << std::endl;
        std::cout << "Данные в stdout буфере (без \\n)";
        
        
        pid_t pid = fork();
        
        if (pid == 0) {
            std::cout << "\nДОЧЕРНИЙ: используем _Exit()" << std::endl;
            
            *logfile << "Дополнительные данные от дочернего" << std::endl;
            std::cout << " + данные от дочернего";
            
            _Exit(42);
            
        } else if (pid > 0) {
            int status;
            wait(&status);
            
            std::cout << "\nРОДИТЕЛЬ: дочерний завершился с кодом " << 
                         WEXITSTATUS(status) << std::endl;
            
            std::cout << "РОДИТЕЛЬ: cleanup_called = " << cleanup_called << std::endl;
            
            *logfile << "Данные от родителя" << std::endl;
            std::cout << " + данные от родителя";
            
            std::cout << "\nРОДИТЕЛЬ: используем exit()" << std::endl;
            exit(0);
            
        } else {
            throw std::runtime_error("fork failed");
        }
    }
    
    void demonstrate_buffer_problem() {
        std::cout << "\n=== Проблема дублирования буферов ===" << std::endl;
        
        std::cout << "Эта строка без \\n в stdout";
        
        std::ofstream file("buffer_test.txt");
        file << "Строка в файловом буфере без flush";
        
        pid_t pid = fork();
        
        if (pid == 0) {
            std::cout << " - из дочернего\n";
            file << " - дополнено дочерним" << std::endl;
            
            exit(0);
            
        } else if (pid > 0) {
            std::cout << " - из родителя\n";
            file << " - дополнено родителем" << std::endl;
            
            wait(nullptr);
            file.close();
            
            std::cout << "Содержимое файла:" << std::endl;
            system("cat buffer_test.txt");
            
        } else {
            throw std::runtime_error("fork failed");
        }
    }
    
    void demonstrate_proper_fork_pattern() {
        std::cout << "\n=== Правильный паттерн fork + exit ===" << std::endl;
        
        std::cout << "Данные в буфере stdout";
        
        std::ofstream file("proper_test.txt");
        file << "Данные в файловом буфере";
        
        std::cout.flush();
        file.flush();
        
        pid_t pid = fork();
        
        if (pid == 0) {
            std::cout << " - дочерний\n";
            file << " - дочерний" << std::endl;
            
            _Exit(0);
            
        } else if (pid > 0) {
            std::cout << " - родитель\n";  
            file << " - родитель" << std::endl;
            
            wait(nullptr);
            file.close();
            
            std::cout << "Содержимое файла (без дублирования):" << std::endl;
            system("cat proper_test.txt");
            
        } else {
            throw std::runtime_error("fork failed");
        }
    }
};

std::ofstream* ExitDemo::logfile = nullptr;
bool ExitDemo::cleanup_called = false;

int main() {
    try {
        ExitDemo demo;
        
        demo.demonstrate_exit_vs_Exit();
        demo.demonstrate_buffer_problem();
        demo.demonstrate_proper_fork_pattern();
        
        std::cout << "\n=== Все демонстрации exit завершены ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
