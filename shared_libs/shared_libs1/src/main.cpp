//
// See the comments in the makefile
//
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <dlfcn.h>
#include <iostream>

int main(int argc, char **argv)
{
	void *handle;
	double (*cosine1)(double);
	char *error;

    //  handle = dlopen("libm.so", RTLD_LAZY);

	//handle = dlopen("./libmym.so", RTLD_LAZY|RTLD_GLOBAL);
	//std::string currdir(get_current_dir_name());
	std::string currdir("./");

	std::string libfilename = currdir + "/libmym.so";
	handle = dlopen(libfilename.c_str(), RTLD_LAZY|RTLD_GLOBAL);
    //  handle = dlopen("./libmym.so", RTLD_LAZY);
	if (!handle) {
	    	fprintf(stderr, "couldn't get the library, sorry: %s\n", dlerror());
    		exit(EXIT_FAILURE);
    	}

	dlerror();    // Clear any existing error 

	// Writing: cosine = (double (*)(double)) dlsym(handle, "cos");
	// would seem more natural, but the C99 standard leaves
	// casting from "void *" to a function pointer undefined.
	// The assignment used below is the POSIX.1-2003 (Technical
    	// Corrigendum 1) workaround; see the Rationale for the
    	// POSIX specification of dlsym().

    	*(void **) (&cosine1) = dlsym(handle, "cos1");

	if ((error = dlerror()) != NULL) {
    		fprintf(stderr, "second error: %s\n", error);
	        exit(EXIT_FAILURE);
    	}

	printf("here is the result number: from the libmym.so %lf\n", (*cosine1)(2.0));
	dlclose(handle);

//---------------------------
	std::string libfilename2 = currdir + "/libmym2.so";

	//handle = dlopen("./libmym2.so", RTLD_LAZY|RTLD_GLOBAL);
	handle = dlopen(libfilename2.c_str(), RTLD_LAZY|RTLD_GLOBAL);
	if (!handle) {
	    	fprintf(stderr, "couldn't get the library, sorry: %s\n", dlerror());
    		exit(EXIT_FAILURE);
 	}
	dlerror();    // Clear any existing error 
	*(void **) (&cosine1) = dlsym(handle, "cos1");

	if ((error = dlerror()) != NULL) {
    		fprintf(stderr, "second error: %s\n", error);
		exit(EXIT_FAILURE);
    	}
	printf("here is the result number: from the libmym2.so %lf\n", (*cosine1)(2.0));
	dlclose(handle);

	exit(EXIT_SUCCESS);
}

