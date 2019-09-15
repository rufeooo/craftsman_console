
#include <stdio.h>
#include <stdlib.h>

#include "functor.h"
#include "rdtsc.h"

size_t
test(size_t a)
{
  getenv("USERFOO");

  return 0;
}

int
main(int argc, char **argv)
{
  const unsigned OneMil = 1 * 1000 * 1000;
  uint64_t start = rdtsc();
  Functor_t f = functor_init(test);
  f.param[0].i = 35;
  for (int i = 0; i < OneMil; ++i) {
    functor_invoke(&f);
  }
  uint64_t end1 = rdtsc();
  for (int i = 0; i < OneMil; ++i) {
    test(35);
  }
  uint64_t end2 = rdtsc();
  for (int i = 0; i < OneMil; ++i) {
    getenv("USERFOO");
  }
  uint64_t end3 = rdtsc();

  printf("\n%lu functor %lu indirect %lu direct\n", end1 - start, end2 - end1,
         end3 - end2);
  return 0;
}
