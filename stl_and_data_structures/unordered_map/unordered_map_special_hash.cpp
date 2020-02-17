// $ g++ -std=c++1z <filename> 
#include <iostream>
#include <string>
#include <unordered_map>

struct X {
    int i,j,k;
    X(int ii, int jj, int kk):i(ii),j(jj),k(kk){}
    bool operator==(const X r) const {
        return i ==r.i && j ==r.j && k ==r.k ;
    }
};

struct hash_X{
  size_t operator()(const X &x) const{
    return std::hash<int>()(x.i) ^ std::hash<int>()(x.j) ^ std::hash<int>()(x.k);
  }
};
 
/* or, set the default hash function for X
namespace std {
    template <>
        class hash<X>{
        public :
        size_t operator()(const X &x ) const{
            return hash<int>()(x.i) ^ hash<int>()(x.j) ^ hash<int>()(x.k);
        }
    };
} */
 
int main() {
    std::unordered_map<X,int,hash_X> my_map;
   // std::unordered_map<X,int> my_map; ---- if the default is used
    X oneX(1,2,3);
    my_map[oneX] = 123;
    my_map[X(22,33,44)] = 234;
    my_map[X(22,133,144)] = 1234;
    
    std::cout << my_map[X(22,133,144)] << "\n";
    std::cout << my_map[X(22,33,44)] << "\n";
        
    std::cout << "\nThe whole thing:\n";
    for(auto m : my_map) {
        std::cout << m.first.i << " -> " << m.second << "\n";
    }
    return 0;
}
