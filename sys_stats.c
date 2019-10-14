
#include "sys_stats.h"
#include "macro.h"
#include "xs_float.h"

static double
tsc_to_float(uint64_t tsc)
{
  int32_t interval = tsc >> 4;
  return xs_float(interval) * 16.0;
}

void
sys_statsInit(Stats_t *accum)
{
  accum->min = UINT64_MAX;
  accum->max = 0;
  for (int i = 0; i < ARRAY_LENGTH(accum->moments); ++i) {
    accum->moments[i] = 0.0;
  }
}

double
sys_statsSamples(Stats_t *accum)
{
  return accum->moments[0];
}

double
sys_statsMean(Stats_t *accum)
{
  return accum->moments[1];
}

double
sys_statsRsDev(Stats_t *accum)
{
  double mean = sys_statsMean(accum);
  return sys_statsVariance(accum) / (mean * mean);
}

double
sys_statsVariance(Stats_t *accum)
{
  return accum->moments[2] / accum->moments[0];
}

double
sys_statsMin(Stats_t *accum)
{
  return tsc_to_float(accum->min);
}

double
sys_statsMax(Stats_t *accum)
{
  return tsc_to_float(accum->max);
}

static void
add_sample(Stats_t *accum, double newValue)
{
  // Calculate
  double n = sys_statsSamples(accum);
  double n1 = n + 1.0;
  double delta = newValue - sys_statsMean(accum);
  double delta1 = delta / n1;
  double delta2 = delta1 * delta * n;

  // Apply
  accum->moments[0] += 1.0;
  accum->moments[1] += delta1;
  accum->moments[2] += delta2;
}

void
sys_statsAddSample(Stats_t *accum, uint64_t sample)
{
  add_sample(accum, tsc_to_float(sample));
  accum->max = MAX(sample, accum->max);
  accum->min = MIN(sample, accum->min);
}

