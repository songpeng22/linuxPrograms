#include <iostream>
using namespace std;

#define _XOPEN_SOURCE 700
#include <assert.h>
#include <signal.h>
#include <stdbool.h> /* false */
#include <stdio.h> /* perror */
#include <stdlib.h> /* EXIT_SUCCESS, EXIT_FAILURE */
#include <sys/wait.h> /* wait, sleep */
#include <unistd.h> /* fork, write */

#define KERN_SOH	"\001"		/* ASCII Start Of Header */
#define KERN_SOH_ASCII	'\001'

#define KERN_EMERG	KERN_SOH "0"	/* system is unusable */
#define KERN_ALERT	KERN_SOH "1"	/* action must be taken immediately */
#define KERN_CRIT	KERN_SOH "2"	/* critical conditions */
#define KERN_ERR	KERN_SOH "3"	/* error conditions */
#define KERN_WARNING	KERN_SOH "4"	/* warning conditions */
#define KERN_NOTICE	KERN_SOH "5"	/* normal but significant condition */
#define KERN_INFO	KERN_SOH "6"	/* informational */
#define KERN_DEBUG	KERN_SOH "7"	/* debug-level messages */

#define KERN_DEFAULT	KERN_SOH "d"	/* the default kernel loglevel */

#define CONCATENATION_STRING_1	"123" "45"
#define CONCATENATION_STRING_2(A,B)	A ## B

int main() {
    cout << "main()" << endl;

    //normal concatenation
    cout << CONCATENATION_STRING_1 << endl;
    cout << CONCATENATION_STRING_2(66,88) << endl;

    //linux concatenation
    cout << "KERN_DEBUG:" << KERN_DEBUG << endl;
    cout << "KERN_DEFAULT:" << KERN_DEFAULT << endl;

    return EXIT_SUCCESS;
}
