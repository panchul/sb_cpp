#
#
#g++ -c file1.cpp -c file2.cpp -c main.cpp -c main3.cpp
#ar rv libstaticlib.a file1.o file2.o
##g++ -o withstatic main.cpp libstaticlib.a
#g++ -o withstatic main.o libstaticlib.a
#g++ -shared -o libsharedlib.so file1.o file2.o
#gcc -Xlinker -rpath ./ -L./ -lsharedlib -o withshared main.o
#gcc -Xlinker -rpath ./ -L./ -lsharedlib3 -o withshared3 main.o
#g++ -shared -nostartfiles -o libsharedlib3.so file3.o
#
#


all:
	mkdir -p obj
	rm -f obj/*
	g++ -Wall -fPIC -c src/mylib1.cpp -o obj/mylib1.o
	g++ -Wall -shared -nostartfiles -o obj/libsharedlib1.so obj/mylib1.o
	g++ -Wall -fPIC -c src/mylib2.cpp -o obj/mylib2.o
	g++ -Wall -shared -nostartfiles -o obj/libsharedlib2.so obj/mylib2.o
	g++ -Wall -rdynamic -ldl -o obj/withshared src/main.cpp
	#ld -rpath ./  -L./ -lsharedlib -o obj/withshared obj/main.o

