// from here: http://www.vishalchovatiya.com/7-best-practices-for-exception-handling-in-cpp-with-example/
// Will only work with C++14 or above

// $ g++ -std=c++17 move_if_noexcept.cpp
// $ ./a.out                                    
//   Moving
//   Moving
//
     
#include <iostream>
#include <string>
#include <vector>
#include <exception>
#include <stdexcept>

using namespace std;

struct demo
{
    demo() = default;
    demo(const demo &) { cout << "Copying\n"; }
    // Exception safe move constructor
    demo(demo &&) noexcept { cout << "Moving\n"; }
private:
    std::vector<int>    m_v;
};

int main()
{
    demo obj1;
    if (noexcept(demo(std::declval<demo>()))){  // if moving safe
        demo obj2(std::move(obj1));             // then move it
    }
    else{
        demo obj2(obj1);                        // otherwise copy it
    }
    demo obj3(std::move_if_noexcept(obj1));     // Alternatively you can do this----------------
    return 0;
}
    