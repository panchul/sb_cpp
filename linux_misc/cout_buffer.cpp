//
// Quck demo of output buffers flushed _after_ fork, because there is no "\n" at the end, or explicit flush().
//
//$ g++ cout_buffer.cpp 
//$ ./a.out 
//HelloHello
//
//
#include <stdio.h>
#include <unistd.h>
int main() {
  printf("Hello");
  fork();
  return 0;
}
