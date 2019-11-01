#include <fstream>
#include <iostream>

using namespace std;

int main()
{
    ofstream f("tmp_myfile.txt");
    f << "Some text to file" << endl;
    cout << "And some to console" << endl;
    return 0;
}
