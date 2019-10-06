#include <iostream>
#include <vector>
 
int main()  {
    std::vector<int> v = {7, 5, 16, 8};
 
    v.push_back(25);
    v.push_back(13);
 
    for(auto n : v) {
        std::cout << n << '\n';
    }
        
    std::cout << "again \n";
    
    for(std::vector<int>::const_iterator it=v.begin(); it != v.end(); it++) {
        std::cout << *it << '\n';
    }
}

