// demonstrates emplace vs insert
// $ g++ -std=c++1z multiset.cpp 

#include <iostream>
#include <set>

using namespace std;
  
int main()
{
	// declaring map
	multiset<pair<char, int> > ms;
      
	// using emplace() to insert pair in-place
	ms.emplace('a', 24);
      
	// Below line would not compile
	// ms.insert('b', 25);    
      
	// using emplace() to insert pair in-place
	ms.insert(make_pair('b', 25));    
      
	// printing the multiset
	for (auto it = ms.begin(); it != ms.end(); ++it)
        	cout << " " << (*it).first << " "
             		<< (*it).second << endl;
  
	return 0;
}


