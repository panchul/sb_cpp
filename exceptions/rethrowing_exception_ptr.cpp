// from here: http://www.vishalchovatiya.com/7-best-practices-for-exception-handling-in-cpp-with-example/
// Will only work with C++14 or above

// $ g++ -std=c++17 rethrowing_exception_ptr.cpp
// $ ./a.out                                    
//   exception: func1() failed
//    exception: func2() failed
//     exception: TESTING NESTED EXCEPTION SUCCESS
     
#include <iostream>
#include <string>
#include <exception>
#include <stdexcept>

void print_nested_exception(const std::exception_ptr &eptr=std::current_exception(), size_t level=0)
{
    static auto get_nested = [](auto &e) -> std::exception_ptr {
        try { return dynamic_cast<const std::nested_exception &>(e).nested_ptr(); }
        catch (const std::bad_cast&) { return nullptr; }
    };
    try{
        if (eptr) std::rethrow_exception(eptr);
    }
    catch (const std::exception &e){
        std::cerr << std::string(level, ' ') << "exception: " << e.what() << '\n';
        print_nested_exception(get_nested(e), level + 1);// rewind all nested exception
    }
}
// -----------------------------------------------------------------------------------------------
void func2(){
    try         { throw std::runtime_error("TESTING NESTED EXCEPTION SUCCESS"); }
    catch (...) { std::throw_with_nested(std::runtime_error("func2() failed")); }
}
void func1(){
    try         { func2(); }
    catch (...) { std::throw_with_nested(std::runtime_error("func1() failed")); }
}
    
int main()
{
    try                             { func1(); }
    catch (const std::exception&)   { print_nested_exception(); }
    return 0;
}
    
    