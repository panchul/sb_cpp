// for_each example
// for one of the cases I used lambda, so might have to use standard specifier for C++17
// g++ --std=c++17 for_each.cpp 
//

#include <iostream>     // std::cout
#include <algorithm>    // std::for_each
#include <vector>       // std::vector

void myfunction (int i) {  // function:
  std::cout << ' ' << i;
}

struct myclass {           // function object type:
  void operator() (int i) {std::cout << ',' << i;}
} myobject;

int main () {
  std::vector<int> myvector;
  myvector.push_back(10);
  myvector.push_back(20);
  myvector.push_back(30);

  std::cout << "myvector contains:";
  for_each (myvector.begin(), myvector.end(), myfunction);
  std::cout << '\n';

  // or:
  std::cout << "myvector contains:";
  for_each (myvector.begin(), myvector.end(), myobject);
  std::cout << '\n';

  std::cout << "myvector contains:";
  for_each (myvector.begin(), myvector.end(), [](auto i){std::cout << i << "..";});
  std::cout << '\n';


  return 0;
}