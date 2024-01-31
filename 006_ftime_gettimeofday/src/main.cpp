#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <stdint.h>

int main()
{
    struct timeb tp;
    struct timespec ts;
    struct timeval tv;
    char buff[100];

    //ftime
    ftime(&tp);
    //timespec
    timespec_get(&ts, TIME_UTC);
    //gettimeofday
    gettimeofday(&tv, NULL);

    printf("timespec.tv_sec:Raw timespec.time_t: %jd\n", (intmax_t)ts.tv_sec);
    printf("timespec.tv_nsec:Raw timespec.tv_nsec: %09ld\n", ts.tv_nsec);

    printf("gettimeofday.tv_sec: %jd\n", (intmax_t)tv.tv_sec);
    printf("gettimeofday.tv_usec*1000: %09ld\n", tv.tv_usec*1000);

    printf("timeb.time:%d\n", tp.time);
    printf("timeb.millitm*1000000:%d\n", tp.millitm * 1000000);

    strftime(buff, sizeof buff, "%D %T", gmtime(&ts.tv_sec));
    printf("Current time: %s.%09ld UTC\n", buff, ts.tv_nsec);

    return 0;
}
