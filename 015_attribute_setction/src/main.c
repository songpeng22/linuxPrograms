#include <stdio.h> /* perror */

#define MAX_TEXT_LEN    1024
int main() {
    char text[MAX_TEXT_LEN];
    int len = 0;

    //usage of strstr
    memset(text,0,MAX_TEXT_LEN);
    len = sprintf(text,"[    0.099220] schedtune: verify normalization");
    printf("text:%s\n",text);

    return 0;
}

void test_normal_function()
{
    printf("this is test_normal_function().\n");
}

void __attribute__((__section__(".init.text"))) test_setction()
{
    printf("this is test_setcion().\n");
}
