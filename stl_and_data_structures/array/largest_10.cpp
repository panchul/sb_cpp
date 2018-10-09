//
// findis 10 biggest integers within 100,000 array, having some room for parallelism.
//
// c++17 features
// g++ -std=c++1z largest_10.cpp
//

#include <functional>
#include <iostream>
#include <queue>
#include <vector>

using namespace std;

#define MYSIZE	100000
#define NBUCKETS 100
#define BUCKET_SIZE (MYSIZE / NBUCKETS)

void find_10(const int *input, int *result) {
	priority_queue<int> myres;
		
	for(int i = 0; i < BUCKET_SIZE; ++i) {
		myres.push(input[i]);
	}

	cout << "result : ";
	for(int i = 0; i < 10; ++i) {
		cout << myres.top() << " ";
		result[i] = myres.top();
		myres.pop();
	}
	cout << "\n";
}

int main() {

	int input_array[MYSIZE];
	
	for(int i=0; i< MYSIZE; ++i) {
		input_array[i] = i;
	}	

	int bucket[NBUCKETS * 10];

	// we can run these in parallel.
	for (int i = 0; i < NBUCKETS; ++i) {
		cout << "bucket " << i << "\n";
		find_10(&input_array[BUCKET_SIZE*i], &bucket[i*10]);
	}

	cout << "now going over the buckets...\n";
	int result[10];
	find_10(bucket, result);

	cout << "result:\n";
	for(int i=0; i<10; ++i) {
		cout << result[i] << " ";
	}
	cout << "\n";

	return 0;
}


