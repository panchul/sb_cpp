// Specify that it's for C++17 like so:
//
// $ g++ -std=c++1z misc_cpp17.cpp
//
#include <iostream>
//#include <conio.h>
#include <algorithm>    // std::find_if
#include <functional>
#include <vector>      

#define somefunc(...) {std::cout << __VA_ARGS__ << "\n";}

//------------------------------------------
void fff(int, const int(&)[2] = {}) { }   // #1
void fff(const int&, const int(&)[1]) { }   // #2
void test() {
    const int x = 17;
    //auto g = [](auto a) {     <--- the example did not have '=' and said it would work(?) c++11?
    auto g = [=](auto a) {
        fff(x);  // OK: calls #1, does not capture x
    };
    auto g2 = [=](auto a) {
         int selector[sizeof(a) == 1 ? 1 : 2]{};
         fff(x, selector);  // OK: is a dependent expression, so captures x
    };
}

//------------------------------------------
// C# thingie:
//var filteredElements = elements.Where(elm = > elm.filterProperty == true);

template<typename T>
std::vector<T> select_T(std::vector<T> inVec, std::function<bool(const T&)> predicate)
{
    std::vector<T> result;
    copy_if(inVec.begin(), inVec.end(), back_inserter(result), predicate);
    return result;
}
 
void doThingie()
{
    std::vector<int> mVec = { 1,4,7,8,9,0 };
    // filter out values > 5
    auto gtFive = select_T<int>(mVec, [](auto a) {return (a > 5); });

    // or > target
    int target = 5;
    //auto gt = select_T<int>(mVec, [target](auto a) {return (a > target); });
    std::vector<int> gt = select_T<int>(mVec, [target](auto a) {return (a > target); });

    std::for_each(gt.begin(), gt.end(), [=](auto a) {std::cout << a << " "; });
}
//------------------------------------------

int main() {
    unsigned int aaa = 0b1010'1010;
    std::cout << "here's my binary var: 0x" << std::hex << aaa << std::dec << "\n";
 
    #ifdef ttt__STDC_HOSTED__
      std::cout << "here's __STDC_VERSION__: \"" << __STDC_VERSION__ << "\"\n";
      std::cout << "here's my binary var: 0x" << std::hex << aaa << std::dec << "\n";
    #endif

    somefunc("govno");
    // 'Identity' is a lambda that accepts an argument of any type and
    // returns the value of its parameter. 
    auto Identity = [](auto a) { return a; };
    int three = Identity(3);
    char const* hello = Identity("hello");

    // Conversion to function pointer for capture-less lambdas
    int(*fpi)(int) = Identity;
    char(*fpc)(char) = Identity;
    //----------------------------------------------------------------

    auto glambda = [](auto a, auto&& b) { return  a < b; };
    bool b = glambda(3, 3.14);       // OK
    // generic lambda
    auto vglambda = [](auto printer) {
        return [=](auto&& ... ts) {   // OK: ts is a  function parameter pack
            printer(std::forward<decltype(ts)>(ts)...);
            return [=]() {
                printer(ts ...);
            };
        };
    };

    auto p = vglambda([](auto v1, auto v2, auto v3)
                { std::cout << v1 << v2 << v3 << "\n"; });
 
    auto q = p(1, 'a', 3.14);  // OK: outputs 1a3.14
    q();                       // OK: outputs 1a3.14
 
    //----------------------------------------------------------------
    auto GL = [](auto a) { std::cout << a; return a; };
    int(*GL_int)(int) = GL;  // OK: through conversion function template
    GL_int(3);                // OK: same as GL(3)   

    //------------------------------------------
    // gcc and clang
    __attribute__((deprecated("message about deprecation"))) int a;

    // Visual Studio
    //__declspec(deprecated("message about deprecation")) int a;
    #pragma warning(suppress : 4996)
    a = 9;
// A more radical
//#pragma warning(disable : 4996)
    //------------------------------------------
    /*
    template <typename T>
    std::function<T(T)> makeConverter(T factor, T offset) {
        return [=](T input) -> T { return (offset + input) * factor; };
    }
    auto milesToKm = makeConverter(1.60936, 0.0);
    */

    //------------------------------------------
    int bbb = 1'123;
    std::cout << "\n bbb " << bbb << "\n";
    //------------------------------------------

#define MMM(x, ...) __VA_ARGS__
    int x[2] = { MMM(1'2,3'4) };
    //int x[2] = {}; // C++2011
    //int x[2] = { 3’4 }; // Now
    std::cout << "x[0] " << x[0] << "\n";
    std::cout << "x[1] " << x[1] << "\n";

    //------------------------------------------
    {
        std::vector<int> v = { 1,2,3,4,5,6,7 };

        std::for_each(v.begin(), v.end(), [=](int y) {std::cout << y << " "; });

        std::vector<int>::iterator res = std::find_if(v.begin(), v.end(), [](int a) {return a % 2; });

        std::cout << "res " << *res << "\n";
    }
    //------------------------------------------

    doThingie();

    //------------------------------------------
    //getch();

    return 0;
}
