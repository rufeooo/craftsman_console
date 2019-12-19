#include <stdint.h>
#include <stdio.h>

#include "rdtsc.h"

int
main()
{
  printf("%lu\n", rdtsc());
  return 0;
}
