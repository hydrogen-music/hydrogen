#include <stdio.h>
#include <time.h>
#include <stdint.h>
int main( int argc, char** argv, char** env ) {
    uint64_t dt;
    struct timespec t0,t1;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    sleep(1);
    clock_gettime(CLOCK_MONOTONIC, &t1);

    dt = ((t1.tv_sec * 1000000000) + t1.tv_nsec) - ((t0.tv_sec * 1000000000) + t0.tv_nsec);

    //fprintf(stdout,"delta is : %4d [ms] => %7d [us] => %10d [ns]\n",(int)(dt/1000000), (int)(dt/1000), (int)dt);

    return ( (int)(dt/1000000)>950 ) ? 0 : 1;
}
