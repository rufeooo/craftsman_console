#pragma once

#include <math.h>

typedef struct {
  double moments[3];
  double min;
  double max;
} Stats_t;

double
stats_count(const Stats_t *accum)
{
  return accum->moments[0];
}

double
stats_mean(const Stats_t *accum)
{
  return accum->moments[1];
}

double
stats_variance(const Stats_t *accum)
{
  return accum->moments[2] / accum->moments[0];
}

double
stats_unbiased_rs_dev(const Stats_t *accum)
{
  return sqrt(accum->moments[2] / (accum->moments[1] * accum->moments[1] *
                                   (accum->moments[0] - 1)));
}

double
stats_min(const Stats_t *accum)
{
  return accum->min;
}

double
stats_max(const Stats_t *accum)
{
  return accum->max;
}

void
stats_init(Stats_t *accum)
{
  *accum = (Stats_t){.min = ~0u};
}

void
stats_init_array(unsigned n, Stats_t stats[static n])
{
  for (int i = 0; i < n; ++i) {
    stats_init(&stats[i]);
  }
}

void
stats_add(double sample, Stats_t *accum)
{
  // Calculate
  double n = accum->moments[0];
  double n1 = n + 1.0;
  double diff_from_mean = sample - accum->moments[1];
  double mean_accum = diff_from_mean / n1;
  double delta2 = mean_accum * diff_from_mean * n;

  // Apply
  accum->moments[0] += 1.0;
  accum->moments[1] += mean_accum;
  accum->moments[2] += delta2;

  // Min/max
  accum->max = fmax(sample, accum->max);
  accum->min = fmin(sample, accum->min);
}
