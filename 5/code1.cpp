#include <iostream>
#include <string>
#include <locale>

int main() {
    std::setlocale(LC_ALL, "");  // или например "ru_RU.UTF-8"
    
    std::string ascii_str = "Hello, world!";
    std::string utf8_str = "Привет, мир!";  // UTF-8 в исходном коде
    
    std::cout << ascii_str << std::endl;
    std::cout << utf8_str << std::endl;
    
    return 0;
}
