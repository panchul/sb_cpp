//
// Neat tricks to conveniently initialize a vector.
// $ g++ -std=c++1z vector_init.cpp 
//

#include <iostream>
#include <vector>

template <typename T>

class make_vector {
private:
  std::vector<T> data_;
  
public:
  
  typedef make_vector<T> my_type;
  
  operator std::vector<T>() const {
    return data_;
  }

  my_type& operator<< (const T& val) {
    data_.push_back(val);
    return *this;
  }
};


// for 5
template <typename T, size_t N>
T* begin(T(&arr)[N]) { return &arr[0]; }
template <typename T, size_t N>
T* end(T(&arr)[N]) { return &arr[0]+N; }


int main(int argc, char* argv[]) {

    std::vector<int> v = make_vector<int>() << 1 << 2 << 3;

    std::cout << "v size is " << v.size() << "; contains : " << v[0] << ", etc." << std::endl;

    //std::vector<int> v2 = v;
    //std::cout << "v2 size is " << v2.size() << " contains : " << v2[0] << " etc." << std::endl;

    // This wants C++11 flags for the compiles ( g++ -std=c++11 ... )
    // 
    //std::vector<int> v3 { 34,23 };
    //// or
    //// std::vector<int> v3 = { 34,23 };

    int vv[2] = { 12,43 };
    std::vector<int> v4(&vv[0], &vv[0]+2);
    std::cout << "v4 size is " << v4.size() << "; contains : " << v4[0] << ", etc." << std::endl;

    //--------------------------
    // 5 ( needs the templates above.

    int vv5[] = { 125,435 };
    std::vector<int> v5(begin(vv5), end(vv5));

    std::cout << "v5 size is " << v5.size() << "; contains : " << v5[0] << ", etc." << std::endl;
    //-----------------------------------

    return 0;
}
