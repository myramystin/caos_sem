#include <iostream>
#include <dlfcn.h>

int main() {
    void* handle = dlopen("./libmath.so", RTLD_LAZY);
    if (!handle) {
        std::cerr << "Cannot load library: " << dlerror() << std::endl;
        return 1;
    }
    
    typedef int (*math_func_t)(int, int);
    
    math_func_t add_func = (math_func_t) dlsym(handle, "add_numbers");
    math_func_t mult_func = (math_func_t) dlsym(handle, "multiply_numbers");
    
    if (!add_func || !mult_func) {
        std::cerr << "Cannot load symbols: " << dlerror() << std::endl;
        dlclose(handle);
        return 1;
    }
    
    std::cout << "5 + 3 = " << add_func(5, 3) << std::endl;
    std::cout << "5 * 3 = " << mult_func(5, 3) << std::endl;
    
    dlclose(handle);
    return 0;
}
