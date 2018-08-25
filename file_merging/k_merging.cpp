
#include <iostream>
#include <fstream>
#include <string>

using namespace std;


void mymerge(const string& f1, const string& f2, const string& fout) {
	
	ifstream fs1(f1);
	ifstream fs2(f2);
	ofstream fsout(fout);
	//istream_iterator eof;

	int next1, next2;

	fs1 >> next1;
	fs2 >> next2;

	while(!fs1.eof() && !fs2.eof()) {
		if(next1 <= next2) {
			fsout << next1 << ' ';
			fs1 >> next1;	
		} else {
			fsout << next2 << ' '; 
			fs2 >> next2;
		}				
	}
	
	while(!fs1.eof()) {
		fsout << next1 << ' ';
		fs1 >> next1;
	}
	while(!fs2.eof()) {
		fsout << next2 << ' ';
		fs2 >> next2;
	}
}


int main(int argc, char *argv[]){

	string fin[5], fout;
	
	for(int i = 0; i< 5 ; ++i) {
		cin >> fin[i];
	}	
	cin >> fout;
	
	mymerge(fin[0], fin[1], "~merge0_1");	
	mymerge(fin[2], fin[3], "~merge2_3");	
	mymerge(fin[4], "~merge2_3", "~merge2_3_4");	
	mymerge("~merge0_1", "~merge2_3_4", fout);	

	return 0;
}
