
#include <iostream>

extern "C" double cos1(double myarg);

extern "C"
{
/// !!!! gotta have this to be able to use cout stuff
// put 'exter int __dso_handle;' somewhere to find out
// where this variable is defined (the error message will tell)

	void *__dso_handle = NULL;
}

double cos1(double myarg)
{
	std::cout << " here I am inside the func in mym2.cpp\n\n";	
	return 456.0;
}

