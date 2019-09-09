#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "rdtsc.h"

unsigned long long tscPerSec() {
  unsigned long long start = rdtsc();
  unsigned long long stop = 0;

  clock_t cStart = clock();
  while ((clock() - cStart) * 1000 / CLOCKS_PER_SEC < 1) {
    stop = rdtsc();
  }

  return stop - start;
}

int main() {
  for (;;) {
    unsigned long long tps = tscPerSec();

    printf("%llu elapsed in one millisecond\n", tps);
  }

  /*for (;;) {
    start = stop;
    clock_t t = clock();
    stop = rdtsc();
    printf("%llu\n", stop - start);
    usleep(1000 * 23);
  }*/
}
