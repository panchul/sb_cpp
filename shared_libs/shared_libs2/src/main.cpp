// from http://www.ibm.com/developerworks/linux/library/l-dll.html
//
//     Test Linux Dynamic Function Loading					
//
//     void       *dlopen(const char *filename, int flag)	
//     		Opens dynamic library and return handle			
//
//     const char *dlerror(void)							
//         	Returns string describing the last error.		
//
//     void       *dlsym(void *handle, char *symbol)		
//         	Return pointer to symbol's load point. 			
//         	If symbol is undefined, NULL is returned.		
//
//     int        dlclose (void *handle) 					
//         	Close the dynamic library handle.				
//
#include <stdio.h>
#include <stdlib.h>

#include "mylib_api.h"

//					
// 1-dll include file and variables	
//					
#include <dlfcn.h>

extern "C" {
	void  *FunctionLib;		//  Handle to shared lib file	
	int   (*Function1)();		//  Pointer to loaded routine	can have any return value, and any arguments
	int   (*Function2)();		//  Pointer to loaded routine	
	const char *dlError;		//  Pointer to error string	
}

typedef int (*myFunc)();

int main()
{
	int   rc;
	printf("Hi there, I am main()\n");
//                                               
//  3-Open Dynamic Loadable Libary with absolute path      
//                                              
  FunctionLib = dlopen("./libsharedlib1.so", RTLD_LAZY); // maybe absolute path would be better
  dlError = dlerror();
  printf("	dlTest  3-Open Library with absolute path return-\"%s\"- \n", dlError);
  if( dlError ) exit(1);

// 4-Find the first loaded function	
  Function1 = (myFunc)dlsym( FunctionLib, "sayhello1");
  dlError = dlerror();                     
  printf("	dlTest  4-Find symbol sayhello1 return-\"%s\"- \n", dlError);
  if( dlError ) exit(1);

// 5-Execute the first loaded function				
//  rc = (*Function1)( HelloMessage );
  (*Function1)();
//  printf("	dlTest  5-printUPPERCASE return-%s- \n", dlError);

// 6-Close the shared library handle					
// Note:  after the dlclose, "printUPPERCASE" is not loaded		
  rc = dlclose(FunctionLib);
  dlError = dlerror();
  printf("	dlTest  6-Close handle return-%s-\n", dlError); 
  if( rc ) exit(1);

//////////////////////////// same with the other file
//                                               
//  3-Open Dynamic Loadable Libary with absolute path      
//                                              
  FunctionLib = dlopen("./libsharedlib2.so", RTLD_LAZY); // maybe absolute path would be better
  dlError = dlerror();
  printf("	dlTest  3-Open Library with absolute path return-\"%s\"- \n", dlError);
  if( dlError ) exit(1);

// 4-Find the first loaded function	
  Function1 = (myFunc)dlsym( FunctionLib, "sayhello1");
  dlError = dlerror();                     
  printf("	dlTest  4-Find symbol sayhello1 return-\"%s\"- \n", dlError);
  if( dlError ) exit(1);

// 5-Execute the first loaded function				
//  rc = (*Function1)( HelloMessage );
  (*Function1)();
//  printf("	dlTest  5-printUPPERCASE return-%s- \n", dlError);

// 6-Close the shared library handle					
// Note:  after the dlclose, "printUPPERCASE" is not loaded		
  rc = dlclose(FunctionLib);
  dlError = dlerror();
  printf("	dlTest  6-Close handle return-%s-\n", dlError); 
  if( rc ) exit(1);

//	sayhello1();
//	sayhello2();
	return 0;
}

