#pragma once

#include <stdint.h>
#include <stdlib.h>

uint64_t memhash(const void *buf, size_t len);
uint64_t memhash_cont(uint64_t seed, const void *buf, size_t len);

