#include <iostream>
#include <vector>

void functionC() {
    int *ptr = nullptr;
    std::cout << "Сейчас произойдет segmentation fault..." << std::endl;
    *ptr = 10;
}

void functionB() {
    functionC();
}

void functionA() {
    std::vector<int> vec = {1, 2, 3};
    std::cout << "Размер вектора: " << vec.size() << std::endl;
    functionB();
}

int main() {
    functionA();
    std::cout << "Эта строка не будет выведена" << std::endl;
    return 0;
}
