#include <iostream>
using namespace std;

#include <stdio.h> /* perror */
#include <string.h>     //support for memset
#include <assert.h>
#include <signal.h>
#include <stdbool.h> /* false */
#include <stdlib.h> /* EXIT_SUCCESS, EXIT_FAILURE */
#include <sys/wait.h> /* wait, sleep */
#include <unistd.h> /* fork, write */

#define MAX_TEXT_LEN    1024
int main() {
    char text[MAX_TEXT_LEN];
    char text_tmp[MAX_TEXT_LEN];
    int len = 0;
    int tmp_len = 0;
    bool bFound = false;

    //usage of strstr
    memset(text,0,MAX_TEXT_LEN);
    memset(text_tmp,0,MAX_TEXT_LEN);
    strcpy(text,"[    0.099220] schedtune: verify normalization");
    char * resultStr = strstr(text,"verify normalization");
    printf("resultStr:%s\n",resultStr);
    if(resultStr != NULL)
        bFound = true;

    printf("bFound:%d.\n",bFound);

    //usage of sprintf
    memset(text,0,MAX_TEXT_LEN);
    memset(text_tmp,0,MAX_TEXT_LEN);
    tmp_len = sprintf(text,"[    0.099220] schedtune: verify normalization");
    printf("text:%s\n",text);

    return EXIT_SUCCESS;
}
