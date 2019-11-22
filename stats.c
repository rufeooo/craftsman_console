#include "stats.h"
#include "macro.h"

static void
add_sample(double newValue, Stats_t *accum)
{
  // Calculate
  double n = stats_sample_count(accum);
  double n1 = n + 1.0;
  double delta = newValue - stats_mean(accum);
  double delta1 = delta / n1;
  double delta2 = delta1 * delta * n;

  // Apply
  accum->moments[0] += 1.0;
  accum->moments[1] += delta1;
  accum->moments[2] += delta2;
}

void
stats_init(Stats_t *accum)
{
  *accum = (Stats_t){ .min = ~0u };
}

void
stats_init_array(unsigned n, Stats_t stats[static n])
{
  for (int i = 0; i < n; ++i) {
    stats_init(&stats[i]);
  }
}

void
stats_sample_add(double sample, Stats_t *accum)
{
  add_sample(sample, accum);
  accum->max = MAX(sample, accum->max);
  accum->min = MIN(sample, accum->min);
}

double
stats_sample_count(const Stats_t *accum)
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
stats_rs_dev(const Stats_t *accum)
{
  double mean = stats_mean(accum);
  return stats_variance(accum) / (mean * mean);
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

