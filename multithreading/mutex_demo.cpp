// baseline is from here: http://www.cplusplus.com/reference/mutex/mutex/
// $ g++ -std=c++1z mutex_demo.cpp -lpthread
#include <iostream>
#include <thread>
#include <mutex>

#define USE_MUTEX

#ifdef USE_MUTEX
std::mutex mtx;
#endif

void print_block(int n, char c)
{
#ifdef USE_MUTEX
    mtx.lock();
#endif
    for (int i = 0; i < n; ++i)
    {
        std::cout << c << '\n';
    }
    std::cout << '\n';
#ifdef USE_MUTEX
    mtx.unlock();
#endif
}

int main()
{
#ifdef USE_MUTEX
    std::cout << "with mutex:\n";
#else
    std::cout << "without mutex:\n";
#endif

    std::thread th1(print_block, 50, '1');
    std::thread th2(print_block, 50, '2');

    th1.join();
    th2.join();

    return 0;
}
