#include <iostream>
using namespace std;
int main()
{
	cout << "biggest integer on a 16-bit machine is \"" << 0xffff << "\"\n";
	cout << "                 (which is, if signed, \"" << (short int)0xffff << "\")\n";
	cout << "                   so for signed it is \"" << (short int)0x7fff << "\"\n";
	cout << "                     1 + that would be \"" << (short int)(0x7fff + 1) << "\"\n";
	return 0;
}
