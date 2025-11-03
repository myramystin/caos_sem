#include <iostream>

int main() {
    int a = 10, b = 20, result;

    // GNU inline assembly позволяет вставлять ассемблерный код прямо в C++ программы. 
    // Синтаксис: asm [volatile] ( "assembly code" : output : input : clobbers ).
    
    asm volatile (
        "addl %1, %0"           // assembly template
        : "=r" (result)         // output operands
        : "r" (a), "0" (b)      // input operands
        :                       // clobbered registers
    );
    
    std::cout << "Result: " << result << std::endl;
    
    int x = 5, y = 3;
    int multiply_result, add_result;

    // %0, %1, %2 - ссылки на операнды по порядку
    // "=r" - output в любом регистре
    // "r" - input в любом регистре
    // volatile предотвращает оптимизации компилятора
    
    asm volatile (
        "movl %2, %%eax\n\t"    // загрузить x в eax
        "imull %3, %%eax\n\t"   // умножить на y
        "movl %%eax, %0\n\t"    // сохранить результат умножения
        "addl %3, %0"           // добавить y к результату
        : "=&r" (multiply_result), "=r" (add_result)
        : "r" (x), "r" (y)
        : "eax"
    );
    
    std::cout << "x * y + y = " << multiply_result << std::endl;

    x = 15, y = 10;
    int sub_result;
    bool is_negative;
    
    asm volatile (
        "subl %2, %0\n\t"       // вычитаем y из x (x - y), кладем в 0
        "sets %1"               // устанавливаем байт если знаковый флаг установлен
        : "=&r" (sub_result),   // early clobber для результата
          "=r" (is_negative)    // флаг отрицательности
        : "r" (y), "0" (x)      // y в регистре, x в том же что sub_result
        : "cc"                  // изменяем флаги
    );

    std::cout << "Subtraction: " << sub_result 
              << ", is negative: " << (bool)is_negative << std::endl;

    float vec1[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float vec2[4] = {5.0f, 6.0f, 7.0f, 8.0f};
    float result_vec[4];

    asm volatile (
        "movups %1, %%xmm0\n\t"     // загружаем vec1 в xmm0
        "movups %2, %%xmm1\n\t"     // загружаем vec2 в xmm1  
        "addps %%xmm1, %%xmm0\n\t"  // складываем векторы
        "movups %%xmm0, %0"         // сохраняем результат
        : "=m" (result_vec)
        : "m" (vec1), "m" (vec2)
        : "xmm0", "xmm1"
    );
    
    std::cout << "SIMD addition: ";
    for (int i = 0; i < 4; i++) {
        std::cout << result_vec[i] << " ";
    }
    std::cout << std::endl;
    return 0;
}
