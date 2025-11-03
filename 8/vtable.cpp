#include <iostream>
#include <cstdio>
#include <cstdint>

int add(int a, int b) { return a + b; }
int multiply(int a, int b) { return a * b; }

class Base {
public:
    virtual void func1() { std::cout << "Base::func1\n"; }
    void func2() { std::cout << "Base::func2\n"; }
    virtual ~Base() = default;
};

class Derived : public Base {
public:
    void func1() override { std::cout << "Derived::func1\n"; }
    void func2() { std::cout << "Derived::func2\n"; }
    void func3() { std::cout << "Derived::func3\n"; }
};

void print_vtable(Base* obj) {
    uintptr_t* vtable_ptr = *reinterpret_cast<uintptr_t**>(obj);
    
    printf("Object address: %p\n", obj);
    printf("VTable address: %p\n", vtable_ptr);
    printf("func1 address: %p\n", reinterpret_cast<void*>(vtable_ptr[0]));
    printf("func2 address: %p\n", reinterpret_cast<void*>(vtable_ptr[1]));
    printf("destructor address: %p\n", reinterpret_cast<void*>(vtable_ptr[2]));
}

int main() {
    // Указатели на функции
    int (*operation)(int, int);
    
    operation = add;
    std::cout << "5 + 3 = " << operation(5, 3) << std::endl;
    
    operation = multiply;  
    std::cout << "5 * 3 = " << operation(5, 3) << std::endl;
    
    Base base_obj;
    Derived derived_obj;
    
    std::cout << "\nBase object vtable:\n";
    print_vtable(&base_obj);
    
    std::cout << "\nDerived object vtable:\n";  
    print_vtable(&derived_obj);
    
    // Полиморфный вызов
    Base* ptr = &derived_obj;
    
    // ptr->func3(); // не работает 

    // ptr - указатель на память derived


    ptr->func1(); // Вызовется Derived::func1 через vtable

    ptr->func2(); // Вызовется Base::func2 через vtable
    
    return 0;
}
