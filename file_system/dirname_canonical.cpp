//
// g++ -std=c++17
//

#include <iostream>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

int main()
{

	fs::path tmp = "c:\\temp\\";

	cout << "initial tmp: \"" << tmp << "\"\n";

	tmp = fs::canonical(tmp); // will remove slash

	cout << "canonical tmp: \"" << tmp << "\"\n";

	fs::path dir_name = tmp.filename(); // will get temp

	cout << "dir_name: \"" << dir_name << "\"\n";

	return 0;
}
