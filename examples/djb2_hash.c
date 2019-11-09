#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

uint64_t hash_val = 5381;

size_t
hash_add(int val)
{
  hash_val = (hash_val << 5) + hash_val + val;

  return hash_val;
}

