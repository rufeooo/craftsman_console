#pragma once

#include <stdint.h>

// Prefer determinism over speed
#pragma STDC FP_CONTRACT OFF

#define INLINE __attribute__((always_inline))

INLINE float
signed_to_float(int64_t i)
{
  float f = i;
  return f;
}

INLINE double
signed_to_double(int64_t i)
{
  double d = i;
  return d;
}

INLINE float
to_float(uint64_t i)
{
  float f = i;
  return f;
}

INLINE double
to_double(uint64_t i)
{
  double d = i;
  return d;
}

