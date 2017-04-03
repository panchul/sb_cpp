
// compile it with 'g++ dump_env.cpp

#include <stdio.h>

int main(int argc, char **argv, char **envp)
{
     printf("Using int main(int argc, char **argv, char **envp) :\n-------------\n");

     char *p;
     while ((p = *envp++))
         printf("%s\n", p);

     printf("-------------\n");
     return 0;
}


//int main()
//{
//    char **ep = environ;
//    char *p;
//    while ((p = *ep++))
//         printf("%s\n", p);
//    return 0;
//}


