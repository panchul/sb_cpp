
#include <iostream>
#include <queue>

using namespace std;

int main ()
{
	queue<int> myqueue;
	
	myqueue.push (1);
	myqueue.push (2);
	myqueue.push (3);
	myqueue.push (4);

  	cout << "myqueue: ";
	
	while (!myqueue.empty())
	{
		cout << ' ' << myqueue.front();
		myqueue.pop();
  	}
	cout << '\n';

  	return 0;
}

