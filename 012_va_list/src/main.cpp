#include <stdio.h>
#include <stdarg.h>
#include <math.h>
 
double stddev(int count, ...) 
{
    double sum = 0;
    double sum_sq = 0;
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; ++i) {
        double num = va_arg(args, double);
        sum += num;
    }
    va_end(args);
    return (sum/count);
}
 
int main(void) 
{
    printf("%f\n", stddev(4, 1.0, 2.0, 3.0, 4.0));
}
