//
// mylib2.cpp
// the second file, out of two, adhering to the same api
//

#include <stdio.h>
#include "mylib_api.h"

class class_one {
public:
	void say_hi() {
		printf("Hi there, I am say_hi() from class_one in mylib2.cpp\n");
		return;
	}
} myclass1;

extern "C" int sayhello1()
{
	myclass1.say_hi();
	return 0;
}

class class_two {
public:
	void say_hi() {
		printf("Hi there, I am say_hi()  from class_two in mylib2.cpp\n");
		return;
	}
} myclass2;

extern "C" int sayhello2() {
	myclass2.say_hi();
	return 0;
}
