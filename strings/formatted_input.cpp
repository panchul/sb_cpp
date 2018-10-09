
// compile it with g++
//
// $ g++ formatted_input.cpp
//

#include <string>
#include <iostream>

#include <sstream> // for 2

using namespace std;

void formatted_input1()
{
     cout << "Demoing formatted input1. using string and substr()" << endl;
     string s = "\":48754+7812=Abcs\"";
     cout << "input string: \"" << s << "\"" << endl;
     string::size_type idx = s.find(":");
     string::size_type end_first = s.find("+", idx + 1);
     string f_number = s.substr(idx + 1, end_first - (idx + 1));
     string::size_type end_second = s.find("=", end_first + 1);
     string s_number = s.substr(end_first + 1, end_second - (end_first + 1));
     string::size_type string_end = s.find("\"", end_second);
     string str = s.substr(end_second + 1, string_end - (end_second + 1));
     cout << "f_number: " << f_number << " s_number: " << s_number << " str: \"" << str << "\"" << endl;
     cout << endl;
}

void formatted_input2()
{
     cout << "Demoing formatted input2. Using stringstream" << endl;
     string str = ":12341+414112=absca";
     cout << "input string: \"" << str << "\"" << endl;
     stringstream ss(str);
     int v1, v2; 
     char col, op, eq; 
     std::string var;
     ss >> col >> v1 >> op >> v2 >> eq >> var;
     cout << "v1: " << v1 << " v2: " << v2 << " var: \"" << var << "\"" << endl;
     cout << endl;
}

void formatted_input3()
{
     cout << "Demoing formatted input3. Using scanf" << endl;
     char a[101], b[111], c[121];
     char input_string[]=":48754+7812=Abcs";
     cout << "input string: \"" << input_string << "\"" << endl;
     sscanf(input_string, ":%100[^+]+%110[^=]=%120s", a, b, c);
     string sa(a), sb(b), sc(c);
     cout << "sa: "<< sa << " sb: " << sb  << " sc: " << sc << endl;
     cout << endl;
}

int main()
{
     formatted_input1();
     formatted_input2();
     formatted_input3();
     
     return 0;
}

