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

void signal_handler(int sig) {
    char s1[] = "SIGUSR1\n";
    char s2[] = "SIGUSR2\n";
    char s3[] = "SIGSEGV\n";
    if (sig == SIGUSR1) {
        write(STDOUT_FILENO, s1, sizeof(s1));       //write SIGUSR1 to stdout
        cout << "signal_handler::SIGUSR1" << endl;
    } else if (sig == SIGUSR2) {
        write(STDOUT_FILENO, s2, sizeof(s2));
        cout << "signal_handler::SIGUSR2" << endl;
    } else if (sig == SIGSEGV) {
        write(STDOUT_FILENO, s3, sizeof(s3));
        cout << "signal_handler::SIGSEGV" << endl;
    }
    signal(sig, signal_handler);
}

int main() {
    cout << "main()" << endl;

    pid_t pid;

    //catch specified signal, set signal handler
    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);
    signal(SIGSEGV, signal_handler);
    //pid = fork();
    pid = getpid();
    cout << "pid:" << pid << endl;
    if (pid == -1) {
        perror("fork");
        assert(false);
    } 
    else 
    {
        if (pid == 0) {
            while (1);
            exit(EXIT_SUCCESS);
        }
        while (1) 
        {
            //send signal SIGUSR1
            kill(pid, SIGUSR1);
            sleep(1);
            //send signal SIGUSR2
            kill(pid, SIGUSR2);
            sleep(1);
            //send signal SIGSEGV
            kill(pid, SIGSEGV);
            sleep(1);
        }
    }
    return EXIT_SUCCESS;
}
