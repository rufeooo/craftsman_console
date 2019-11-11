#include <stdio.h>
#include <stdlib.h>

#include "../float.h"

double djb2_double = 5381;

size_t
fp_hash(ApiDouble_t d)
{
  djb2_double = (djb2_double * 5) + djb2_double + d.value;
  printf("%f\n", djb2_double);
  return 1;
}

size_t
fp_add(ApiDouble_t d, ApiDouble_t dd, ApiDouble_t ddd)
{
  double sum = d.value + dd.value + ddd.value;
  printf("%f\n", sum);

  return 1;
}
