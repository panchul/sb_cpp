/*

Entering a char, a word, and a sentense.

In order to take a line as input, you can use scanf("%[^\n]%*c", s);
where is defined as char s[MAX_LEN] where is the maximum size of.
Here, [] is the scanset character. ^\n stands for taking input until a
newline isn't encountered. Then, with this %*c, it reads the newline
character and here, the used * indicates that this newline character
is discarded.

Input:
C
word
some sentence with white spaces.

Output:

C
word
some sentence with white spaces.

*/


#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define MAX_LEN 8192

int main() 
{
    char ch;
    scanf("%c", &ch);
    printf("%c\n", ch);
    scanf("\n");
    
    char s[MAX_LEN];
    scanf("%[^\n]%*c", s);
    printf("%s\n", s);
    scanf("\n");
    
    char sen[MAX_LEN];
    scanf("%[^\n]%*c", sen);
    printf("%s\n", sen);
    
    return 0;
}

