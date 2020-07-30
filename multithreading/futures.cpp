// g++ futures.cpp -pthread -std=c++1y

#include <future>
#include <iostream>
using namespace std;
void func1()
{
    this_thread::sleep_for(chrono::seconds(1));
}

int main()
{
    auto f = async([]() { return 3; });
    cout << f.get();
}
