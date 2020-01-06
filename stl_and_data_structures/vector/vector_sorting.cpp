// $ g++ -std=c++1z vector_init.cpp 
#include <iostream>
#include <vector>

#include <algorithm> // some compilers want this too

int main() {
    std::vector<int> myvec = { 1,3,5,6,2,3,4,5};
    
    std::cout << "Default comparator:\n";
    std::sort(myvec.begin(), myvec.end());
    for(auto i : myvec) {
        std::cout << i << " ";
    }

    std::cout << "\nCustom comparator:\n";
    std::sort(myvec.begin(), myvec.end(),[](const auto& x, const auto&y){return x>y;});
    std::for_each(myvec.begin(), myvec.end(), [=](const auto& y) {std::cout << y << ", "; });
    std::cout << "\n";

    std::vector<std::string> mySvec = { "lala", "baba", "aa"};
        
    std::cout << "Default comparator:\n";
    std::sort(mySvec.begin(), mySvec.end());
    for(auto i : mySvec) {
        std::cout << i << " ";
    }
    
    std::cout << "\nCustom comparator:\n";
    std::sort(mySvec.begin(), mySvec.end(),[](const auto& x, const auto&y){return x[0]>y[0];});
    for(auto i : mySvec) {
        std::cout << i << " ";
    }
    std::cout << "\n";
    return 0;
}