#include <stdio.h>
#include "../shared_library/cal.h"

int main(int argc, char *argv[])
{
    int a = 2, b = 1, c = 100;
    printf("a+b=%d.\n", cal(0, a,b));
    return 0;
}

