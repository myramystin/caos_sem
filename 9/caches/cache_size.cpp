#include <iostream>
#include <chrono>
#include <vector>
#include <random>

#ifdef __x86_64__
#include <cpuid.h>

struct CacheInfo {
    int level;
    int type;  // 1=Data, 2=Instruction, 3=Unified
    int size;
    int associativity;
    int line_size;
};

void get_cache_info() {
    unsigned int eax, ebx, ecx, edx;
    
    std::cout << "Cache Information:" << std::endl;
    std::cout << "==================" << std::endl;
    
    for (int i = 0; i < 4; ++i) {
        eax = 4;  // CPUID leaf 4 - Cache information
        ecx = i;  // Cache level
        
        __cpuid_count(4, i, eax, ebx, ecx, edx);
        
        int cache_type = eax & 0x1F;
        if (cache_type == 0) break;  // No more caches
        
        int level = (eax >> 5) & 0x7;
        int line_size = (ebx & 0xFFF) + 1;
        int partitions = ((ebx >> 12) & 0x3FF) + 1;
        int ways = ((ebx >> 22) & 0x3FF) + 1;
        int sets = ecx + 1;
        
        int cache_size = ways * partitions * line_size * sets;
        
        const char* type_str[] = {"", "Data", "Instruction", "Unified"};
        
        std::cout << "L" << level << " " << type_str[cache_type] 
                  << " Cache: " << cache_size / 1024 << " KB" << std::endl;
        std::cout << "  Line size: " << line_size << " bytes" << std::endl;
        std::cout << "  Associativity: " << ways << "-way" << std::endl;
        std::cout << "  Sets: " << sets << std::endl << std::endl;
    }
}
#endif

int main() {
#ifdef __x86_64__
    get_cache_info();
#else
    std::cout << "CPUID not available on this architecture" << std::endl;
#endif
    return 0;
}
