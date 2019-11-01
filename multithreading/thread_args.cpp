// $ g++ -std=c++1z thread_args.cpp -lpthread
// $ g++ -std=c++1z thread_args.cpp -pthread

#include <iostream>
#include <thread>
#include <mutex>

using namespace std;

void tfunc(int aaa, char bbb, string sss) {
	cout << "aaa " << aaa << " bbb " << bbb << " sss " << sss << "\n";
}

int main() {
	thread a(tfunc, 1,'3', "govno1" ), b(tfunc, 2, '7', "govno2");

	a.join();
	b.join();

	return 0;
}

