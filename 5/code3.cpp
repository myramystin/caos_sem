#include <iostream>
#include <string>
#include <vector>

std::vector<std::string> split_utf8(const std::string& str) {
    std::vector<std::string> result;
    
    for (size_t i = 0; i < str.length();) {
        int cp_bytes = 1;
        
        if ((str[i] & 0xE0) == 0xC0) cp_bytes = 2;      // 110xxxxx - начало 2-байтовой последовательности
        else if ((str[i] & 0xF0) == 0xE0) cp_bytes = 3; // 1110xxxx - начало 3-байтовой последовательности
        else if ((str[i] & 0xF8) == 0xF0) cp_bytes = 4; // 11110xxx - начало 4-байтовой последовательности
        
        if (i + cp_bytes > str.length()) break;
        
        result.push_back(str.substr(i, cp_bytes));
        i += cp_bytes;
    }
    
    return result;
}

int main() {
    std::setlocale(LC_ALL, "");
    
    std::string text = "Hello, Привет, 你好!";
    auto chars = split_utf8(text);
    
    std::cout << "Всего байт: " << text.length() << std::endl;
    std::cout << "Всего символов: " << chars.size() << std::endl;
    
    for (const auto& ch : chars) {
        std::cout << ch << " (" << ch.length() << " байт)" << std::endl;
    }
    
    return 0;
}
