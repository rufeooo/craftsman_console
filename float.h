#pragma once

#include <stdint.h>

// Prefer determinism over speed
#pragma STDC FP_CONTRACT OFF

#if _d_BigEndian_
const uint32_t _d_iexp_ = 0;
const uint32_t _d_iman_ = 1;
#else
const uint32_t _d_iexp_ = 1;
const uint32_t _d_iman_ = 0;
#endif // BigEndian_

#ifndef INLINE
#define INLINE __attribute__((always_inline)) inline
#endif

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

INLINE uint64_t
double_uint64(double d)
{
  uint64_t i = d;
  return i;
}

INLINE uint32_t
double_sign(double d)
{
  return ((int32_t *) &d)[_d_iexp_] & 0x80000000;
}

INLINE void
double_signxor(double *d, double o)
{
  ((int32_t *) d)[_d_iexp_] ^= double_sign(o);
}

INLINE uint64_t
double_round_uint64(double d)
{
  double half = .5;
  double_signxor(&half, d);
  uint64_t i = (d + half);
  return i;
}

