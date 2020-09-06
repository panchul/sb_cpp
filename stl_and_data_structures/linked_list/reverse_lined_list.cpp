
#include <iostream>

using namespace std;

class MyNode {
public:
	MyNode():next(NULL), Value(0){}
	MyNode(int newVal):next(NULL), Value(newVal){}
	
	int Value;
	MyNode *next;
};

class MyList {
private:
	MyNode *_root_node;

public:
	MyList():_root_node(NULL){}
	void push_back(int newnumber) {
		if(_root_node) {		
			MyNode *myit=_root_node;

			while(myit->next != NULL)
				myit = myit->next;	

			myit->next = new MyNode(newnumber);

		} else {
			_root_node = new MyNode(newnumber);
		}
	}
	
	void print() const {
		MyNode *myit=_root_node;
		int current_node = 0;
		cout << "List: ";
		while(myit) {
			cout << current_node++ << ":" << myit->Value << (myit->next? ", " : "\n");
			myit = myit->next;
		}
	}

	void reverse() {
		if(!_root_node) return;	

		MyNode* myit = _root_node;
		MyNode* myit_next(myit->next);
		MyNode* myit_next_next(myit_next ? myit->next->next : NULL);

		myit->next = NULL;

		while(myit_next) {
			myit_next->next = myit;
			myit = myit_next;
			myit_next = myit_next_next;
			myit_next_next = (myit_next_next ? myit_next_next->next : NULL);
		}

		_root_node = myit;
	}
};

typedef class MyList MyList;

int main(int argc, char *argv[]) {

	MyList a;

	a.push_back(1);
	a.push_back(2);
	a.push_back(3);
	a.push_back(4);

	cout << "Initial list: \n";
	a.print();

	a.reverse();

	cout << "After reversing: \n";
	a.print();

	return 0;
}
