// $ cat data_in.txt 
// aaa aaa aaa a a a b bb bbb     bkasdgasg

// $ echo data_in.txt data_out.txt | ./a.out
// $ cat data_out.txt 
//   a
//   aaa
//   b
//   bb
//   bbb
//   bkasdgasg

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <iterator>

using namespace std;

int main(int argc, char *argv[])
{
	string from, to;
	cin >> from >> to;

	ifstream is(from.c_str());	// input stream
	istream_iterator<string> ii(is);
	istream_iterator<string> eos;	// input sentinel

	vector<string> b(ii,eos);
	sort(b.begin(), b.end());

	ofstream os(to.c_str());
	ostream_iterator<string> oo(os, "\n");

	unique_copy(b.begin(), b.end(), oo);

	return !is.eof() || !os;
}
