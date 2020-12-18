//
// https://docs.microsoft.com/en-us/cpp/build/walkthrough-creating-and-using-a-dynamic-link-library-cpp
//

#include <iostream>

#include "some_functions.h"

int main()
{
    std::cout << "Using DLL1...\n";

    int mya = 2;
    int myb = 3;

    std::cout << mya << " " << myb << " = " << add_ints(mya, myb) << "\n";

    std::cout << "Done...\n";
}
