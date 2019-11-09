
#include "hash.h"

#define DJB2_CONST 5381

uint64_t
memhash(const void *buf, size_t len)
{
  uint64_t hash_val = DJB2_CONST;
  const char *read_iter = buf;
  const char *read_end = buf + len;
  while (read_iter < read_end) {
    hash_val = ((hash_val << 5) + hash_val) + *read_iter++;
  }

  return hash_val;
}

uint64_t
memhash_cont(uint64_t seed, const void *buf, size_t len)
{
  uint64_t hash_val = seed;
  const char *read_iter = buf;
  const char *read_end = buf + len;
  while (read_iter < read_end) {
    hash_val = ((hash_val << 5) + hash_val) + *read_iter++;
  }

  return hash_val;
}
