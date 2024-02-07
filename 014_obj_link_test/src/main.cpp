#include <iostream>
using namespace std;

#include <assert.h>
#include <signal.h>
#include <stdbool.h> /* false */
#include <stdio.h> /* perror */
#include <stdlib.h> /* EXIT_SUCCESS, EXIT_FAILURE */
#include <sys/wait.h> /* wait, sleep */
#include <unistd.h> /* fork, write */

#include "device.h"

int main() {

    dev_warn(NULL,"hello:%d.",1);

    return EXIT_SUCCESS;
}
