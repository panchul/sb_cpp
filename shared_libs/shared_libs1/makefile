#
#	03/30/2012	alexp	Moved the files onto git repo
#

#
# Just a test modules to "play" with connecting them together.
# Some of the compiler options may or may not work on a
# particular platform.
# For example, on Debian, 64 bit, it wanted -fPIC, while it 
# was just fine without it on 32 bit.

# For the executable to be able to find the .so files, adjust
# environmental variable LD_RUN_PATH 
# or add -Xlinker -rpath=<the path where .so is>
# (the LD_RUN_PATH variable should be set during the compile time)
#


all:
	make clean
	make obj/shared_lib1
	make obj/libmym.so
	make obj/libmym2.so

obj/shared_lib1 : src/main.cpp
	g++ -g -fPIC -rdynamic src/main.cpp -L. -ldl  -o obj/shared_lib1

obj/libmym.so: obj/mym.o
	ld -E -shared -o obj/libmym.so obj/mym.o

obj/libmym2.so: obj/mym2.o
	ld -E -shared -o obj/libmym2.so obj/mym2.o

obj/mym.o: src/mym.cpp
	g++ -g -fPIC -c src/mym.cpp -o obj/mym.o

obj/mym2.o: src/mym2.cpp
	g++ -g -fPIC -c src/mym2.cpp -o obj/mym2.o
#	g++ -shared -nostartfiles -o obj/libmym.so src/mym.cpp -L.

clean:
	mkdir -p obj
	rm -f obj/*
#shared_lib1 obj/libmym.so obj/mym.o


