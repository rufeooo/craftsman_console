#include <stdio.h>
#include <time.h>
#include <unistd.h>

unsigned long long tsc_per_sec() {
  unsigned long long start = __rdtsc();
  unsigned long long stop = 0;

  clock_t cStart = clock();
  while ((clock() - cStart) * 1000 / CLOCKS_PER_SEC < 1) {
    stop = __rdtsc();
  }

  return stop - start;
}

int main() {
  for (;;) {
    unsigned long long tps = tsc_per_sec();

    printf("%llu elapsed in one millisecond\n", tps);
  }

  /*for (;;) {
    start = stop;
    clock_t t = clock();
    stop = __rdtsc();
    printf("%llu\n", stop - start);
    usleep(1000 * 23);
  }*/
}
