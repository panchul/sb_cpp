
// # g++ -std=c++1z ptrs_misc.cpp
 
#include <iostream>

using namespace std;
 
int main() {
 
     char * pbuff1;
     
     // char * const pbuff2;   // EROR: won't compile because it wants default initialization
     char * const pbuff2 ="something"; // OK with a warning: ISO C++11 does not allow conversion from string literal to 'char *const'
                                       //    [-Wwritable-strings]
     
     char something[]="something";
     
     pbuff1 = something;
     // pbuff2 = something; // ERROR: won't compile because pbuff2 is a const.
 
     return 0;
}