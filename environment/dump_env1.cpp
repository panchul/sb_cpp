// compile it with 'g++ dump_env.cpp
#include <stdio.h>
extern char **environ;
int main() {
     printf("Using 'extern char **environ;'\n-------------\n");
     char **ep = environ;
     char *p;
     while ((p = *ep++))
         printf("%s\n", p);
     printf("-------------\n");
     return 0;
}
