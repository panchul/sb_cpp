//
// in1.txt:
// 2 4 6 
//
// in2.txt:
// 1 3 5 7 
    
// out.txt should be:
// 1 2 3 4 5 6 7
    	
#include <iostream>
#include <fstream>
using namespace std;
 
int main()
{
    ifstream fin1, fin2;
    ofstream fout;
    int n1, n2;
 
    fin1.open("in1.txt");
    fin2.open("in2.txt");
    fout.open("out.txt");

    while(fin1 >> n1 && fin2 >> n2)
    {
        if(n1 < n2)
          fout << n1 << ' ';
        else
          fout << n2 << ' ';
     }
 
     while(fin1 >> n1) fout << n1 << ' ';
     while(fin2 >> n2) fout << n2 << ' ';

    return 0;
}   

