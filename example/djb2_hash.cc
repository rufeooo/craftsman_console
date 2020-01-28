#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

uint64_t hash_val = 5381;

size_t
hash_add(int val)
{
  hash_val = (hash_val << 5) + hash_val + val;
  printf("%zu\n", hash_val);

  return hash_val;
}
