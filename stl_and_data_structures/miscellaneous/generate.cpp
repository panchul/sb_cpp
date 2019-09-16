//
// Q: what is the output of this program?
//
// $ g++ generate.cpp 
// $ ./a.out 
//with a and g
//  0 1 2 3 4 5 6 7 8 9 
//
//  Now with b and h
//  h constructor
//  operator(): n is now 0
//  operator(): n is now 1
//  operator(): n is now 2
//  operator(): n is now 3
//  operator(): n is now 4
//  operator(): n is now 5
//  operator(): n is now 6
//  operator(): n is now 7
//  operator(): n is now 8
//  operator(): n is now 9
//  0 1 2 3 4 5 6 7 8 9 

#include <iostream>
#include <algorithm>
#include <iterator>

struct g
{
    g():n(0){}
	int operator()() { return n++; }
	int n;
};

struct h
{
    h():n(0){std::cout << "h constructor\n";}
	int operator()() { std::cout << "operator(): n is now " << n << "\n"; return n++; }
	int n;
};

int main()
{
    std::cout << "with a and g\n";
 	int a[10];
	std::generate(a, a+10, g());
	std::copy(a, a+10, std::ostream_iterator<int>(std::cout, " "));

    std::cout << "\n\nNow with b and h\n";
    // with more tracing info
	int b[10];
	std::generate(b, b+10, h());
	std::copy(b, b+10, std::ostream_iterator<int>(std::cout, " "));

    return 0;
}
 