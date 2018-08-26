
//
// $ g++ -std=c++1z deque.cpp
//


#include <iostream>
#include <deque>

using namespace std;

int main ()
{
  	unsigned int i;

  	deque<int> first;                                // empty deque of ints
  	deque<int> second (4,100);                       // four ints with value 100
	deque<int> third (second.begin(),second.end());  // iterating through second
	deque<int> fourth (third);                       // a copy of third

	// the iterator constructor can be used to copy arrays:
  	int myints[] = {1,2,3,4};
  	deque<int> fifth (myints, myints + sizeof(myints) / sizeof(int) );

  	cout << "fifth: ";
	for (deque<int>::iterator it = fifth.begin(); it!=fifth.end(); ++it)
		cout << ' ' << *it;

	cout << '\n';

	deque<int> mydeque = {10,20,30};
	mydeque.emplace_front (111);
	mydeque.emplace_front (222);
	cout << "mydeque contains:";
	for (auto& x: mydeque)
		cout << ' ' << x;
	cout << '\n';


	return 0;
}

