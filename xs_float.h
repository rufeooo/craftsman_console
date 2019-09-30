// # Float to Int / Int to Float Conversion Methods
// - original source: http://stereopsis.com/sree/fpu2006.html
// - Ported to C ( 2019 ANewton )
#pragma once

#include "xs_float.h"

#include <stdint.h>

#ifndef _xs_DEFAULT_CONVERSION
#define _xs_DEFAULT_CONVERSION 0
#endif //_xs_DEFAULT_CONVERSION

#if _xs_DEFAULT_CONVERSION == 1
#include <math.h>
#endif

#if _xs_BigEndian_
const uint32_t _xs_iexp_ = 0;
const uint32_t _xs_iman_ = 1;
#else
const uint32_t _xs_iexp_ = 1;
const uint32_t _xs_iman_ = 0;
#endif // BigEndian_

const double _xs_doublemagic =
  (6755399441055744.0); // 2^52 * 1.5,  uses limited precisicion to floor
const double _xs_doublemagicdelta = 0.0f;
// (1.5e-8); // almost .5f = .5f + 1e^(number of exp bit)
const double _xs_doublemagicroundeps =
  (.5f - _xs_doublemagicdelta); // almost .5f = .5f - 1e^(number of exp bit)

static inline int32_t
xs_sign(double a)
{
  return ((int32_t *) &a)[_xs_iexp_] & 0x80000000;
}
static inline void
xs_signxor(double *a, double b)
{
  ((int32_t *) a)[_xs_iexp_] ^= xs_sign(b);
}

static inline int32_t
xs_fistp_int(double val)
{
#if _xs_DEFAULT_CONVERSION == 0
  val = val + _xs_doublemagic;
  return ((int32_t *) &val)[_xs_iman_];
#else
  return val > 0 ? floor(val) : ceil(val);
#endif
}

static inline double
xs_float(int32_t val)
{
#if _xs_DEFAULT_CONVERSION == 0
  double ret = _xs_doublemagic;
  ((int32_t *) &ret)[_xs_iman_] = val;
  return ret - _xs_doublemagic;
#else
  return (double){ val };
#endif
}

static inline int32_t
xs_int(double val)
{
#if _xs_DEFAULT_CONVERSION == 0
  double dme = -_xs_doublemagicroundeps;
  xs_signxor(&dme, val);
  return xs_fistp_int(val + dme);
#else
  return (int32_t){ val };
#endif
}

static inline int32_t
xs_floor_int(double val)
{
#if _xs_DEFAULT_CONVERSION == 0
  const double dme = _xs_doublemagicroundeps;
  return xs_fistp_int(val - dme);
#else
  return floor(val);
#endif
}

static inline int32_t
xs_ceil_int(double val)
{
#if _xs_DEFAULT_CONVERSION == 0
  const double dme = _xs_doublemagicroundeps;
  return xs_fistp_int(val + dme);
#else
  return ceil(val);
#endif
}

static inline int32_t
xs_round_int(double val)
{
#if _xs_DEFAULT_CONVERSION == 0
  return xs_fistp_int(val + _xs_doublemagicdelta);
#else
  return floor(val + .5);
#endif
}

