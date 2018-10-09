
//
// in an array find two elements that produce certain number when added.
// 
// $ g++ -std=c++1z sum_of_2.cpp

#include <iostream>
#include <vector>
#include <deque>

using namespace std;

void find2sum(const vector<int>& inp, int n);

void find2sum(const vector<int>& inp0, int n) {
	deque<int> inp;

	cout << "input: ";

	for(auto it : inp0) {
		cout << it << " ";
		inp.push_back(it);
	}

	cout << "\n";

	sort(inp.begin(), inp.end());
	
	cout << "sorted: ";

	for(auto it : inp)
	cout << it << " ";

	cout << "\n";

	while(inp.size() >= 2 && inp.back() > n) {
		inp.pop_back();
	}

	while(inp.size() >= 2) {
		
		if(inp.front() + inp.back() == n) {
			cout << "found: " << inp.front() << " + " << inp.back() << " = " << n << "\n\n";
			return;
		}
		
		if(inp.front() + inp.back() < n) {
			inp.pop_front();
		} else {
			inp.pop_back();
		}
	}

	cout << "not found " << n << "\n\n";
}

int main() {

	vector<int> inp={1,3,4,9,6,7,8,19,9};
	vector<int> inp2={1,4,5,6,7,9,9,10};

	
	find2sum(inp, 10);
	find2sum(inp, 12);

	find2sum(inp2, 12);

	return 0;
}



