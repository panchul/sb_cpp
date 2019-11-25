// Original snippet was from:
// https://en.cppreference.com/w/cpp/language/except_spec

// $ g++ -std=c++11 unexpected_demo.cpp -lpthread 
//
// $ ./a.out 
// That was unexpected
// Abort trap: 6
//
#include <iostream>
#include <exception>
#include <cstdlib>
class X {};
class Y {};
class Z : public X {};
class W {};

void f() throw(X, Y) {
    int n = 0;
    if (n) throw X(); // OK
    if (n) throw Z(); // also OK
    throw W(); // will call std::unexpected()
}

int main() {
  std::set_unexpected([]{
      std::cout << "That was unexpected" << std::endl; // flush needed
      std::abort();
  });
  f();
}
