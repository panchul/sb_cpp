
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>

extern "C" double cos1(double myarg);

extern "C"
{
/// !!!! gotta have this to be able to use cout stuff
// put 'exter int __dso_handle;' somewhere to find out
// where this variable is defined (the error message will tell)

	void *__dso_handle = NULL;
}

using namespace std;

double cos1(double myarg)
{
	extern char **environ;

	std::string aaa;
	char* something = getenv("YARC_INSTALL_PATH");

	if(something)
	aaa = something;

	std::cout << "aaa is " << aaa << "\n";

	printf("aaa is %s", something);

	

	std::cout << " here I am inside the func in mym.cpp\n\n";	
	return 123.0;
}

